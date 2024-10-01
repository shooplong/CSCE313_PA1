/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name: Long Pham
	UIN: 532003876
	Date: 9/29/2024
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <sys/time.h>

using namespace std;

int main (int argc, char *argv[]) {
	int opt;
	// change p, t, and e to -1 because these are invalid values.
	int p = -1; 
	double t = -1;
	int e = -1;
	string filename = "";

	int m = MAX_MESSAGE;

	vector<FIFORequestChannel*> channels;
	bool new_chan = false;

	// system("mkdir -p received");

	//Add other arguments here, need to add -c and -m case
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}	

	//Task 1:
	//Run the server process as a child of the client process, so we will need fork() and then exec()
	
	pid_t pid = fork(); // bruh i had this as int pid for so long

	if (pid < 0){
		EXITONERROR("Child fork didn't work");
	}
	if (pid == 0){ // the child will run this
		char* cmd[] = {(char*)("./server"), (char*)("-m"), (char*)to_string(m).c_str(),nullptr};
		execvp(cmd[0], cmd);
	}


    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);

	// //Task 4:
	// //Request a new channel
	if (new_chan){
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		// write new channel message into pipe
		cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));

		// read response from pipe (can create any static sized char array that fits server response, eg MAX_MESSAGE)
		char newPipeName[1024]; // discord said to do this
		cont_chan.cread(newPipeName, sizeof(newPipeName));


		// create a new FIFORequestChannel object using the name sent by server
		FIFORequestChannel* chan2 = new FIFORequestChannel(newPipeName, FIFORequestChannel::CLIENT_SIDE);

		channels.push_back(chan2);

	}


	// make sure to use the latest / newest chan for our og "chan" variable
	FIFORequestChannel chan = *(channels.back());
	
	//Task 2:
	//Request data points

	// for 1 point:
	if (p != -1 && t != -1 && e != -1){
		char buf[MAX_MESSAGE];
		datamsg x(p, t, e);

		memcpy(buf, &x, sizeof(datamsg));  // copy datamsg into sep buffer then write into pipe
		chan.cwrite(&x, sizeof(datamsg)); // but we can just directly write datamsg into pipe

		double reply;
		chan.cread(&reply, sizeof(double));

		std::cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	// for 1000 points
	else if (p != -1) { // OMG THIS WAS THE ISSUE
	// open x1.csv file under./received/
	// PT said to use ofstream
		ofstream ofs("received/x1.csv");
	if (!ofs.is_open()){
		cerr << "Couldn't open received/x1.csv" << endl;
		return 1;
	}
	// char buf3[MAX_MESSAGE];

	double time = 0.0;
	// iterate 1000 times
	for (int i = 0; i < 1000; i++) {
		// write time into x1.csv (time is 0.004 second deviations)
		
		// prep the request for ecg1, also datamsg is in format of person, time, and ecg1 in this case, it will find it for us
		datamsg msg1(p, time, 1);
		// send request for ecg1 datamsg into pipe, cwrite is in format of void* msgbuf, msg size
		// memcpy(buf3, &msg1, sizeof(datamsg));
		chan.cwrite(&msg1, sizeof(datamsg)); // i was gonna do sizeof(msg1) at first but realized datamsg better b/c more general.
		// read response for ecg1 from pipe
		double ecg1;
		chan.cread(&ecg1, sizeof(double)); // this reads the actual ecg1 value, store that value we got into &ecg1
		// write ecg1 value into x1.csv
		ofs << time << "," << ecg1;

		// prep req ecg2
		datamsg msg2(p, time, 2);
		// send req for ecg1 datamsg into pipe
		// memcpy(buf3, &msg2, sizeof(datamsg));
		chan.cwrite(&msg2, sizeof(datamsg));
		// read response for ecg2 from pipe
		double ecg2;
		chan.cread(&ecg2, sizeof(double));
		// write ecg2 value into x1.csv
		ofs << "," << ecg2 << endl;

		// increment time
		time += 0.004;
	}
	ofs.close();

	}

	// if requesting whole entire file (annoying case)
	else if (filename != ""){
		filemsg fm(0, 0); // req the file length message, size of the thing we working with
		string fname = filename; // 

		// this calculates the file length request message and set up buffer
		int len = sizeof(filemsg) + (fname.size() + 1); // this is just the size of buffer we want to send the file req to the server
		char* buf2 = new char[len];

		// copies filemsg fm into msgBuffer, attach filename to the end of filemsg fm in msgBuffer, then write msgBuffer into pipe
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len); // i want the file length
		

		//Read file length response from server for specified file
		__int64_t file_length; // this is the key thing here! we need it for fullBiome variable later
		chan.cread(&file_length, sizeof(__int64_t)); // put file length into variable file_length
		std::cout << "The length of " << filename << " is " << file_length << endl;

		char* buf3 = new char[m]; // create buffer of size buff capacity (m)

		ofstream ofs("received/" + filename, ios::binary); // open file to write in binary mode
		if (!ofs.is_open()){
			cerr << "Error opening file received/" << fname << endl;
			delete[] buf2;
			delete[] buf3;
			return 1;
 		}
 
		// loop 
		for (int64_t offset = 0; offset < file_length; offset += m){
			filemsg* file_req = (filemsg*)buf2; // notes video said you can just turn anything into a pointer
			file_req->offset = offset;
			// video in notes said to set the length to the minimum between m ( our buffer ) and how many bytes have been written so far
			file_req->length = min(static_cast<__int64_t>(m), file_length - offset);

			chan.cwrite(buf2, len); // send req

			// cread into buf3 length file_req->len
			chan.cread(buf3, file_req->length);

			// write buf3 into file received/filename
			
			ofs.write(buf3, file_req->length);
			

		}

		// for (int i = 0; i < fullBiome; i++){
		// 	// create filemsg for data chunk range
		// 	// assign data chunk range properly so that the data chunk to fetch from does NOT exceed the file length -> my answer: i can just do the biomes that are guaranteed to be full (in relation to buffer_size) and then 
		// 	__int64_t offset = i * m;
		// 	filemsg fm_biome(offset, m);

		// 	// copy fm_biome into buf2 buffer and write into pipe
		// 	len = sizeof(filemsg) + fname.size() + 1; // new buffer size
		// 	buf2 = new char[len];
		// 	memcpy(buf2, &fm_biome, sizeof(filemsg)); // was doing &fullBiome instead of fm_biome and it was messing it up!!!
		// 	strcpy(buf2 + sizeof(filemsg), fname.c_str());

		// 	chan.cwrite(buf2, len); // req for 1 full biome
		// 	delete[] buf2;

		// 	char* buf3 = new char[m]; // make sure to delete this later since we used "new"
		// 	chan.cread(buf3, m);

		// 	ofs.write(buf3, m);
		// 	delete[] buf3;
		// 	// now we have 3 full chunks written. Well not 3 but like full size - how many full chunks we have, next we have to handle the remainder
		// }

		// // handle remainder of file (if there is a remainder, probably tho what are the odds)
		// if (remainder > 0){
		// 	__int64_t offset = fullBiome * m; // so it would be like 3 * 256 = 768, this would be our offset now after doing the guaranteed full biomes
		// 	filemsg fm_remainder(offset, remainder); // very important that we use the remainder here

		// 	int len = sizeof(filemsg) + (fname.size() + 1);
		// 	buf2 = new char[len];

		// 	memcpy(buf2, &fm_remainder, sizeof(filemsg));
		// 	strcpy(buf2 + sizeof(filemsg), fname.c_str());
		// 	chan.cwrite(buf2, len); // req for remainder
		// 	delete[] buf2; 

		// 	char* buf3 = new char[remainder]; // buffer equal to size of the remainder
		// 	chan.cread(buf3, remainder);

		// 	ofs.write(buf3, remainder);
		// 	delete[] buf3;
		// }
		// ofs.close();

		ofs.close();
		delete[] buf2;
		delete[] buf3;
	}

	// if necessary, close and delete the new channel:

	MESSAGE_TYPE quit_m = QUIT_MSG;
	for (unsigned int i = 1; i < channels.size(); ++i){
		channels[i]->cwrite(&quit_m, sizeof(MESSAGE_TYPE));
		delete channels[i];
	}

	// MESSAGE_TYPE quit_m = QUIT_MSG;
	// if (new_chan) {
	// 	// do your close and deletes
		
	// 	channels.back()->cwrite(&quit_m, sizeof(MESSAGE_TYPE));
	// 	delete channels.back(); // free memory
	// 	channels.pop_back();
	// }
	// int status;
	// waitpid(pid, &status, 0);
	cont_chan.cwrite(&quit_m, sizeof(MESSAGE_TYPE));

	wait(nullptr); // wait to prevent zombie kids

}
