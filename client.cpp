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

using namespace std;

// this is for single data point transfer
void requestDataPoint(FIFORequestChannel& chan, int p, double t, int e) {
	char buf[MAX_MESSAGE];
	datamsg x(1, 0.0, 1);

	memcpy(buf, &x, sizeof(datamsg));  // copy datamsg into sep buffer then write into pipe
	chan.cwrite(&x, sizeof(datamsg)); // but we can just directly write datamsg into pipe
	double reply;
	chan.cread(&reply, sizeof(double));

	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
}

// this requests 1000 data points
// request 1000 data points from the server and write the results into received/ x1.csv with same file format as the patient {1-15}.csv files
void requestData(FIFORequestChannel& chan, int p) {
	// open x1.csv file under./received/
	// PT said to use ofstream
	ofstream ofs("received/x1.csv");

	// iterate 1000 times
	for (int i = 0; i < 1000; i++) {
		// write time into x1.csv (time is 0.004 second deviations)
	}

	
}


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	string filename = "";

	int buffer_size = MAX_MESSAGE;

	//Add other arguments here, need to add -c and -m case
	while ((opt = getopt(argc, argv, "p:t:e:f:c:m:")) != -1) {
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
				buffer_size = atoi (optarg);
				break;
		}
	}	

	if (p != 1 && !filename.empty()){ // not necessary for test PT said but just a good little check
		EXITONERROR("Both p and f were specified");
	}
	//Task 1:
	//Run the server process as a child of the client process, so we will need fork() and then exec()
	char* cmd[] = {(char*)"./server", (char*)"-m", (char*)to_string(buffer_size).c_str(),nullptr};
	int pid = fork();

	if (pid < 0){
		EXITONERROR("Child fork didn't work");
	}
	if (pid == 0){ // the child will run this
		execvp(cmd[0], cmd);
	}

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	//Task 4:
	//Request a new channel
	
	//Task 2:
	//Request data points
	if ( p ) {
		if ( t ) requestDataPoint(chan, p, t, e); // this is if we do "./client -p 1 -t 0.000 -e 1"
		else requestData(chan, p); // if we only do "./clienmmt -p 1" then obviously they want all of patient 1
	}

	


	// i moved all these into a function
    // char buf[MAX_MESSAGE];
    // datamsg x(1, 0.0, 1);
	
	// memcpy(buf, &x, sizeof(datamsg));
	// chan.cwrite(buf, sizeof(datamsg));
	// double reply;
	// chan.cread(&reply, sizeof(double));
	// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
	//Task 3:
	//Request files
	filemsg fm(0, 0);
	string fname = "1.csv";
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);

	delete[] buf2;
	__int64_t file_length;
	chan.cread(&file_length, sizeof(__int64_t));
	cout << "The length of " << fname << " is " << file_length << endl;
	
	//Task 5:
	// Closing all the channels
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
