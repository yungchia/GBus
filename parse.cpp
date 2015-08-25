#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ifstream;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <iterator>
using std::istream_iterator;
#include <algorithm>
using std::copy;

enum status {
    STATUS_NOBUS = 0,
    STATUS_INCOMING,
    STATUS_WAITING,
    STATUS_ARRIVED,
};

struct bus_unit {
    int status;
    int waitTime;
    string bus_stop;
};

vector<bus_unit> busData;

void printStringVector(vector<string> *in) {
    for (int i = 0; i<in->size(); i++) {
        cout << "[" << i << "] " << in->at(i) << endl;
    }
}

void printBusUnitVector(vector<bus_unit> *in) {
    bus_unit temp;
    cout << "toal found: " << in->size() << endl;
    for (int i = 0; i<in->size(); i++) {
        temp = in->at(i);
        cout << "status: " << temp.status << " name: " << temp.bus_stop << endl;
    }
}

bool isPlate(string in) {
    std::size_t found = in.find('-');
    if (found != string::npos) {
        //cout << "bingo " << in << endl;
        return true;
    }
    return false;
}

int buildTable(vector<string> *in) {
    string key("更新時間");
    int startIndex = 0;
    for (int i = 0; i<in->size(); i++) {
        if (!in->at(i).find(key)) {
            startIndex = i+1;
//            cout << "[" << i << "] " << "bingo" << endl;
            break;
        }
    }

    for (int i=startIndex; i<in->size()-1; i++) {
        bus_unit temp;
        cout << "[" << i-startIndex << "] " << in->at(i) << endl;
        string nobus("未發車");
        string wait("約");
        string incoming("將到站");

        if (!isPlate(in->at(i))) {
            cout << "in: " << in->at(i) << endl;
            if (!in->at(i).find(nobus)) {
                temp.status = STATUS_NOBUS;
                if (!isPlate(in->at(i+1))) {
                    temp.bus_stop = in->at(i+1);
                    busData.push_back(temp);
                    i++;
                } else {
                    //TODO
                    i++;
                }
            } else if (!in->at(i).find(wait)) {
               temp.status = STATUS_WAITING;
               if (!isPlate(in->at(i+1))) {
                    temp.bus_stop = in->at(i+1);
                    busData.push_back(temp);
                    i++;
                } else {
                    //TODO
                    i++;
                }
            } else if (!in->at(i).find(incoming)) {
               temp.status = STATUS_INCOMING;
               if (!isPlate(in->at(i+1))) {
                    temp.bus_stop = in->at(i+1);
                    busData.push_back(temp);
                    i++;
                } else {
                    //TODO
                    i++;
                }
            } else {
                temp.status = STATUS_ARRIVED;
                temp.bus_stop = in->at(i);
                busData.push_back(temp);
            }
        } else {
            //TODO
        }
    }

    return 0;
}

int main(int argc, char* const argv[]) {
    vector<string> DataArray;

    if (argc == 2) {
        ifstream myfile(argv[1]);
        string inLine;

        if (!myfile) {
            cout << "Error opening output file" << endl;
            system("pause");
            return -1;
        }

        copy(istream_iterator<string>(myfile), istream_iterator<string>(), back_inserter(DataArray));
        
//        printStringVector(&DataArray);

        int rs = buildTable(&DataArray);

        printBusUnitVector(&busData);
    }
    return 0;
}
