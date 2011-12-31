#include <iostream>

#include <dcpp/stdinc.h>
#include <dcpp/FileReader.h>
#include <dcpp/Util.h>
#include <dcpp/MerkleTree.h>
#include <stdlib.h>

#include <boost/date_time/posix_time/ptime.hpp>
using namespace boost::posix_time;

using namespace std;
using namespace dcpp;

int main(int argc, char** argv)
{
	if(argc < 2) {
		cout << "You need to supply a file name" << endl;
		return 1;
	}

	char x[_MAX_PATH] = { 0 };
	if(!_fullpath(x, argv[1], _MAX_PATH)) {
		cout << "Can't get full path" << endl;
		return 1;
	}

	auto direct = argc < 2 ? true : argv[2][0] == '0';
	auto bufSize = argc < 3 ? 0 : Util::toInt(argv[3]);

	try {
		auto start = microsec_clock::universal_time();
		FileReader fr(direct, bufSize);

		TigerTree tt;
		size_t total = 0;
		fr.read(x, [&](void* x, size_t n) {
			tt.update(x, n);
			total += n;
			if(total % (1024*1024) == 0) {
				std::cout << ".";
			}
		});

		cout << endl << Encoder::toBase32(tt.finalize(), TigerTree::BYTES);

		auto diff = (microsec_clock::universal_time() - start).total_microseconds();
		auto s = diff / 1000000.0;
		if(s == 0) s = 1;
		cout << endl << Util::formatBytes(total) << ", " << s << " s, " << Util::formatBytes(total / s) << " b/s" << endl;
	} catch(const std::exception& e) {
		cout << "Error: " << e.what() << endl;
	}

	return 0;
}



