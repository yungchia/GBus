#include <iostream>
using std::cout;
using std::cerr;
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
#include <sstream>
#include <mysql++.h>
using mysqlpp::Connection;
using mysqlpp::Query;
using mysqlpp::StoreQueryResult;
using mysqlpp::Row;


int hour, min, sec = 0;

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
    string bus_plate;
};

string int2str(int &i) {
    string s;
    std::stringstream ss(s);
    ss << i;

    return ss.str();
}

void printStringVector(vector<string> *in) {
    for (int i = 0; i<in->size(); i++) {
        cout << "[" << i << "] " << in->at(i) << endl;
    }
}

void printBusUnitVector(vector<bus_unit> *in) {
    bus_unit temp;
//    cout << "total found: " << in->size() << endl;
    for (int i = 0; i<in->size(); i++) {
        temp = in->at(i);
        if (temp.status == STATUS_NOBUS)
            cout << "status: " << temp.status << " name: " << temp.bus_stop << endl;
        else if (temp.status == STATUS_ARRIVED)
            cout << "status: " << temp.status << " name: " << temp.bus_stop << " bus: " << temp.bus_plate << endl;
        else if (temp.status == STATUS_WAITING)
            cout << "status: " << temp.status << " name: " << temp.bus_stop << " wait: " << temp.waitTime << endl;
        else
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

int buildTable(vector<string> *in, vector<bus_unit> *out) {
    string key("更新時間");
    int startIndex = 0;
    for (int i = 0; i<in->size(); i++) {
        if (!in->at(i).find(key)) {
            startIndex = i+1;
            //cout << "[" << i << "] " << "bingo" << endl;
            // find out update time
            string colon("：");
            string toFind = in->at(i);
            string rest;
            int foundTimes = 0;
            std::size_t foundH, foundM, foundS = 0;
            foundH = toFind.find(colon);
            if (foundH != string::npos) {
                rest.assign(toFind, foundH+3, in->at(i).size());
                toFind = rest;
            }
            foundM = toFind.find(colon);
            if (foundM != string::npos) {
                rest.assign(toFind, foundM+3, in->at(i).size());
                toFind = rest;
            }
            foundS = toFind.find(colon);
            if (foundS != string::npos) {
                rest.assign(toFind, foundM+3, in->at(i).size());
            }

            toFind = in->at(i);
            rest.assign(toFind, foundH+3, foundM);
            std::istringstream (rest) >> hour;

            toFind = rest.assign(toFind, foundH+3, toFind.size());
            rest.assign(toFind, foundM+3, foundS);
            std::istringstream (rest) >> min;

            toFind = rest.assign(toFind, foundM+3, toFind.size());
            rest.assign(toFind, foundS+3, toFind.size());
            std::istringstream (rest) >> sec;

            cout << "update time: " << hour << ":" << min << ":" << sec << endl;
            break;
        }
    }

    for (int i=startIndex; i<in->size()-1; i++) {
        bus_unit temp;
 //       cout << "[" << i-startIndex << "] " << in->at(i) << endl;
        string nobus("未發車");
        string wait("約");
        string min("分");
        string incoming("將到站");
        size_t found = 0;

        if (!isPlate(in->at(i))) {
//            cout << "in: " << in->at(i) << endl;
            if (!in->at(i).find(nobus)) {
                temp.status = STATUS_NOBUS;
                if (!isPlate(in->at(i+1))) {
                    temp.bus_stop = in->at(i+1);
                    out->push_back(temp);
                    i++;
                } else {
                    //TODO
                    i++;
                }
            } else if (!in->at(i).find(wait)) {
               found = in->at(i).find(wait);
               size_t found2 = in->at(i).find(min);
               string s_min;
               s_min.assign(in->at(i), found+3, found2-(found+3));
               std::istringstream (s_min) >> temp.waitTime;

               temp.status = STATUS_WAITING;
               if (!isPlate(in->at(i+1))) {
                    temp.bus_stop = in->at(i+1);
                    out->push_back(temp);
                    i++;
                } else {
                    //TODO
                    i++;
                }
            } else if (!in->at(i).find(incoming)) {
               temp.status = STATUS_INCOMING;
               if (!isPlate(in->at(i+1))) {
                    temp.bus_stop = in->at(i+1);
                    out->push_back(temp);
                    i++;
                } else {
                    //TODO
                    i++;
                }
            } else {
                temp.status = STATUS_ARRIVED;
                temp.bus_stop = in->at(i);
                temp.bus_plate = in->at(i-1);
                out->push_back(temp);
            }
        } else {
            //TODO
        }
    }

    return 0;
}

int storeToDB(vector<bus_unit> *bus) {
    Connection con(false);
    int rs = con.connect("gbus", "localhost", "gbus", "gbus");
    cerr << "connect to DB: rs = " << rs << endl;

    Query query = con.query();
    //query << "INSERT INTO route " << "VALUES ('blue_21', 17, 13, 12);";
    //string insert = "VALUES ('blue_21', " + hour + "," + " " + min + "," + " " + sec + ")" + ";";
    string insert("VALUES ('blue_21', ");
    insert.append(int2str(hour));
    insert.append(", ");
    insert.append(int2str(min));
    insert.append(", ");
    insert.append(int2str(sec));
    insert.append(");");
    query << "INSERT INTO route " << insert;
    query.execute();
    StoreQueryResult res = query.store();
    if (res && res.size() > 0) {
        cout << "insert to db good" << endl;
    } else {
        cerr << "Failed to get item list: " << query.error() << endl;
    }


    //StoreQueryResult res = query.store();

    //cout << "We have : " << endl;
    /*if (res && res.size() > 0) {
        Row row;
        Row::size_type i;
        if (row.size() > 0) {
            for (i = 0; row = res.at(i); ++i) {
                cout << "data: " << row.at(0) << endl;
            }
        }
    }
    else {
        cerr << "Failed to get item list: " << query.error() << endl;
        return -1;
    }*/

    return 0;
}

int main(int argc, char* const argv[]) {
    vector<string> DataArray;
    vector<bus_unit> busData;

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

        int rs = buildTable(&DataArray, &busData);

        printBusUnitVector(&busData);

        DataArray.clear();

        rs = storeToDB(&busData);

        busData.clear();
    }
    return 0;
}
