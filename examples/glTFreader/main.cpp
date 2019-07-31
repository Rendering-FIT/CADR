#include <vulkan/vulkan.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

using namespace std;

using json=nlohmann::json;


int main(int argc,char** argv) {

	try {

		// get file to load
		if(argc<2) {
			cout<<"Please, specify glTF file to load."<<endl;
			return 1;
		}
		string fileName(argv[1]);
		cout<<"Opening file \""<<fileName<<"\"..."<<endl;

		// open file
		ifstream f(fileName);
		f.exceptions(ifstream::badbit);

		// parse json
		json j3;
		f>>j3;

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
