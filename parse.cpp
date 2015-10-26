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
using mysqlpp::SimpleResult;
using mysqlpp::Row;

//char key[11] = {0};
string key;
int date[5] = {0}; //month, day, hour, min, sec
int total_bus;
int total_stop;
string busname;

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

struct busstop_unit {
    int index;
    int eta;
};

struct bus_where {
    string plate;
    int where;
};

vector<busstop_unit> v_busstop;
vector<bus_where> v_buswhere;

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

#if 0
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
#endif

int buildTable_v2(vector<string> *in, vector<bus_unit> *out) {
    if (in->size() > 2) {
        cerr << "file contains multi lines: " << in->size() << endl;
    }
    string line = in->at(0);
    //cout << "target: " << line << endl;

    vector<string> wordVector;
    size_t prev = 0, pos;
    while ((pos = line.find_first_of("{}:[]=,\"", prev)) != string::npos)
    {
        if (pos > prev)
            wordVector.push_back(line.substr(prev, pos-prev));
        prev = pos+1;
    }
    if (prev < line.length())
        wordVector.push_back(line.substr(prev, string::npos));

    for (int i=0;i<wordVector.size()-1;) {
//        cout << wordVector.at(i) << endl;
        string token = wordVector.at(i++);
        if (!token.compare("Etas")) {
//            cout << "found Etas" << endl;
            while (!wordVector.at(i).compare("idx")) {
                i++;
                int index = 0;
                busstop_unit bu;
                std::istringstream (wordVector.at(i++)) >> index;
//                cout << index << " -> ";
                bu.index = index;
                string etas = wordVector.at(i++);
                if (etas.compare("eta"))
                    cerr << "format error" << endl;
                int eta;
                std::istringstream (wordVector.at(i)) >> eta;
//                cout << eta << endl;
                bu.eta = eta;
                v_busstop.push_back(bu);
                if (i < wordVector.size()-1)
                    i++;
            }
        } else if (!token.compare("Buses")) {
//            cout << "found Buses" << endl;
            while (!wordVector.at(i).compare("bn")) {
                i++;
                bus_where bw;
                string plate = wordVector.at(i);
                bw.plate = plate;
                i = i+5;
                string idx = wordVector.at(i++);
                if (idx.compare("idx"))
                    cerr << "format error" << endl;
                int index = 0;
                std::istringstream (wordVector.at(i)) >> index;
                bw.where = index;
                v_buswhere.push_back(bw);
//                cout << plate << " is at index: " << index << endl;
                //cout << i << " " << wordVector.size() << endl;
                if (i < wordVector.size()-1)
                    i++;
            }
        }
    }

    return 0;
}

int storeToDB(vector<bus_unit> *bus) {
    Connection con(false);
    int rs = con.connect("gbus", "localhost", "gbus", "gbus");
//    cerr << "connect to DB: rs = " << rs << endl;

    Query query = con.query();
    string insert("(bus_storetime,month,day,hour,min,sec) VALUES ('");
    insert.append(key);
    insert.append("'");
    for (int i=0;i<5;i++) {
        insert.append(", ");
        insert.append(int2str(date[i]));
    }
    insert.append(");");
    query << "INSERT INTO route " << insert;
    SimpleResult sr = query.execute();
//    cout << "rows = " << sr.rows() << endl;
/*
    StoreQueryResult res = query.store();
    if (res && res.size() > 0) {
        cout << "insert to db good" << endl;
    } else {
        cerr << "Failed to get item list: " << query.error() << endl;
    }
*/
    query.reset();
    insert.clear();
    insert.append("total_stop = ");
    insert.append(int2str(total_stop));
    for (int i=1;i<=total_stop;i++) {
        insert.append(", ");
        insert.append("stopstatus_");
        insert.append(int2str(i));
        insert.append(" = ");
        insert.append(int2str(v_busstop[i-1].eta));
    }

    for (int i=1;i<=total_bus;i++) {
        insert.append(", ");
        insert.append("busstatus_");
        insert.append(int2str(i));
        insert.append(" = ");
        insert.append(int2str(v_buswhere[i-1].where));
        insert.append(", ");
        insert.append("busplate_");
        insert.append(int2str(i));
        insert.append(" = '");
        insert.append(v_buswhere[i-1].plate);
        insert.append("'");
    }

    insert.append(", ");
    insert.append("bus_name = '");
    insert.append(busname);
    insert.append("' ");

    insert.append(" WHERE bus_storetime = '");
    insert.append(key);
    insert.append("'");
    insert.append(";");
    query << "UPDATE route SET " << insert;

/*
    StoreQueryResult res = query.store();
    //res = query.store();
    if (res && res.size() > 0) {
        cout << "insert to db good" << endl;
    } else {
        cerr << "Failed to get item list: " << query.error() << endl;
    }
*/
    sr = query.execute();
//    cout << "rows = " << sr.rows() << endl;
    //query.reset();

    return 0;
}

int main(int argc, char* const argv[]) {
    vector<string> DataArray;
    vector<bus_unit> busData;

    if (argc == 2) {
        ifstream myfile(argv[1]);

        if (!myfile) {
            cout << "Error opening output file: " << argv[1] << endl;
            //system("pause");
            return -1;
        }

        copy(istream_iterator<string>(myfile), istream_iterator<string>(), back_inserter(DataArray));
        myfile.close();

//        printStringVector(&DataArray);

        //int rs = buildTable(&DataArray, &busData);
        int rs = buildTable_v2(&DataArray, &busData);

        char * pch;
        pch = strtok (argv[1],"_/.-");
        int i = 0;
        while (pch != NULL) {
            if (i==1) {
                busname.append(pch);
                key.append(pch);
                key.append("-");
            }
            if (strlen(pch) == 2) {
                key.append(pch);
                date[i-2] = (pch[0] - '0')*10 + (pch[1] - '0');
            }
            i++;
            pch = strtok (NULL, "_/.-");
        }

        cout << "key-> " << key;
/*        for (i=0; i<5;i++) {
            printf("%d ", date[i]);
        }
        cout <<endl;
        cout << busname << endl;
*/
        cout << " [ " << v_busstop.size();
        cout << " / " << v_buswhere.size() << "]" << endl;
        total_stop = v_busstop.size();
        total_bus = v_buswhere.size();

        rs = storeToDB(&busData);

        DataArray.clear();
        busData.clear();
    }
    return 0;
}
