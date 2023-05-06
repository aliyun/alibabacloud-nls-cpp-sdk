/*
* Copyright 2015 Alibaba Group Holding Limited
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include <windows.h>
#include <ctime>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include "nlsOpuCoder.h"

using std::map;
using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;


#define FRAME_SIZE 640


void work(char* fileName) {

	int errorCode = 0;
	OpusEncoder* encoder = createOpuEncoder(16000, &errorCode);
	if (!encoder) {
		cout << "createOpuEncoder failed." << endl;
		return;
	}

	ifstream fs;
	fs.open(fileName, ios::binary | ios::in);
	if (!fs) {
		cout << fileName << " isn't exist.." << endl;

		return ;
	}

	unsigned char outputBuffer[320] = { 0 };
	int outputSize = 320;
	int encoderSize = 0;

	while (!fs.eof()) {
		char data[FRAME_SIZE] = { 0 };

		fs.read(data, sizeof(char) * FRAME_SIZE);
		int nlen = (int)fs.gcount();

		encoderSize = opuEncoder(encoder, (unsigned char *)data, FRAME_SIZE, outputBuffer, 320);

		cout << "Encoder Size: " << encoderSize << endl;

		//exg: send outputBuffer
	}

	// 关闭音频文件
	fs.close();

	destroyOpuEncoder(encoder);

	return ;
}

int main(int arc, char* argv[]) {

	work("test0.wav");

	return 0;
}
