/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "GeoIP.h"

#include "File.h"
#include "format.h"
#include "SettingsManager.h"
#include "Util.h"
#include "ZUtils.h"

#include <GeoIP.h>
#include <GeoIPCity.h>

namespace dcpp {

GeoIP::GeoIP(string&& path) : geo(0), path(move(path)) {
	if(File::getSize(path) > 0 || decompress()) {
		open();
	}
}

GeoIP::~GeoIP() {
	Lock l(cs);
	close();
}

const string& GeoIP::getCountry(const string& ip) const {
	Lock l(cs);
	if(geo) {
		auto i = cache.find((city() ? (v6() ? GeoIP_record_id_by_addr_v6 : GeoIP_record_id_by_addr) :
			(v6() ? GeoIP_id_by_addr_v6 : GeoIP_id_by_addr))(geo, ip.c_str()));
		if(i != cache.end()) {
			return i->second;
		}
	}
	return Util::emptyString;
}

void GeoIP::update() {
	Lock l(cs);

	close();

	if(decompress()) {
		open();
	}
}

void GeoIP::rebuild() {
	Lock l(cs);
	if(geo) {
		city() ? rebuild_cities() : rebuild_countries();
	}
}

bool GeoIP::decompress() const {
	if(File::getSize(path + ".gz") <= 0) {
		return false;
	}

	try { GZ::decompress(path + ".gz", path); }
	catch(const Exception&) { return false; }

	return true;
}

void GeoIP::open() {
#ifdef _WIN32
	geo = GeoIP_open(Text::toT(path).c_str(), GEOIP_STANDARD);
#else
	geo = GeoIP_open(path, GEOIP_STANDARD);
#endif
	if(geo) {
		GeoIP_set_charset(geo, GEOIP_CHARSET_UTF8);
	}
}

void GeoIP::close() {
	cache.clear();

	GeoIP_delete(geo);
	geo = 0;
}

namespace {

string forwardRet(const char* ret) {
	return ret ? ret : Util::emptyString;
}

#ifdef _WIN32
string getGeoInfo(int id, GEOTYPE type) {
	id = GeoIP_Win_GEOID_by_id(id);
	if(id) {
		tstring str(GetGeoInfo(id, type, 0, 0, 0), 0);
		str.resize(std::max(GetGeoInfo(id, type, &str[0], str.size(), 0), 1) - 1);
		if(!str.empty()) {
			return Text::fromT(str);
		}
	}
	return Util::emptyString;
}
#endif

void countryParams(ParamMap& params, int id) {
	params["2code"] = [id] { return forwardRet(GeoIP_code_by_id(id)); };
	params["3code"] = [id] { return forwardRet(GeoIP_code3_by_id(id)); };
	params["continent"] = [id] { return forwardRet(GeoIP_continent_by_id(id)); };
	params["engname"] = [id] { return forwardRet(GeoIP_name_by_id(id)); };
#ifdef _WIN32
	params["name"] = [id]() -> string {
		auto str = getGeoInfo(id, GEO_FRIENDLYNAME);
		return str.empty() ? forwardRet(GeoIP_name_by_id(id)) : str;
	};
	params["officialname"] = [id]() -> string {
		auto str = getGeoInfo(id, GEO_OFFICIALNAME);
		return str.empty() ? forwardRet(GeoIP_name_by_id(id)) : str;
	};
#else
	/// @todo any way to get localized country names on non-Windows?
	params["name"] = [id] { return forwardRet(GeoIP_name_by_id(id)); };
	params["officialname"] = [id] { return forwardRet(GeoIP_name_by_id(id)); };
#endif
}

} // unnamed namespace

void GeoIP::rebuild_cities() {
	cache.clear();

	const auto& setting = SETTING(COUNTRY_FORMAT);

	GeoIPRecord* record = nullptr;
	auto id = GeoIP_init_record_iter(geo);
	auto prev_id = id;
	while(!GeoIP_next_record(geo, &record, &id) && record) {

		ParamMap params;
		countryParams(params, record->country);

		params["areacode"] = [record] { return Util::toString(record->area_code); };
		params["city"] = [record] { return forwardRet(record->city); };
		params["lat"] = [record] { return Util::toString(record->latitude); };
		params["long"] = [record] { return Util::toString(record->longitude); };
		params["metrocode"] = [record] { return Util::toString(record->metro_code); };
		params["postcode"] = [record] { return forwardRet(record->postal_code); };
		params["region"] = [record]() -> string {
			auto country = GeoIP_code_by_id(record->country);
			auto region = record->region;
			if(country && region) {
				auto str = GeoIP_region_name_by_code(country, region);
				if(str) { return str; }
			}
			return forwardRet(region);
		};

		cache[prev_id] = Util::formatParams(setting, params);
		prev_id = id;

		GeoIPRecord_delete(record);
	}
}

void GeoIP::rebuild_countries() {
	cache.clear();

	const auto& setting = SETTING(COUNTRY_FORMAT);

	for(int id = 1, size = GeoIP_num_countries(); id < size; ++id) {

		ParamMap params;
		countryParams(params, id);

		cache[id] = Util::formatParams(setting, params);
	}
}

bool GeoIP::v6() const {
	return geo->databaseType == GEOIP_COUNTRY_EDITION_V6 || geo->databaseType == GEOIP_LARGE_COUNTRY_EDITION_V6 ||
		geo->databaseType == GEOIP_CITY_EDITION_REV1_V6 || geo->databaseType == GEOIP_CITY_EDITION_REV0_V6;
}

bool GeoIP::city() const {
	return geo->databaseType == GEOIP_CITY_EDITION_REV0 || geo->databaseType == GEOIP_CITY_EDITION_REV1 ||
		geo->databaseType == GEOIP_CITY_EDITION_REV1_V6 || geo->databaseType == GEOIP_CITY_EDITION_REV0_V6;
}

} // namespace dcpp
