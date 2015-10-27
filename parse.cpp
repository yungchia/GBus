#include <unistd.h>
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

string key;
string busname;
int date[5] = {0}; //month, day, hour, min, sec
int total_bus;
int total_stop;

bool debug;

struct busstop_unit {
    int index;
    int eta;
};

struct bus_where {
    string plate;
    int where;
};

struct bus_unit {
    int status;
    int waitTime;
    string bus_stop;
    string bus_plate;
};

vector<busstop_unit> v_busstop;
vector<bus_where> v_buswhere;

string int2str(int &i) {
    string s;
    std::stringstream ss(s);
    ss << i;

    return ss.str();
}

int parseString(string in, string token, vector<string> *out) {
    size_t prev = 0, pos;
    while ((pos = in.find_first_of(token.c_str(), prev)) != string::npos)
    {
        if (pos > prev)
            out->push_back(in.substr(prev, pos-prev));
        prev = pos+1;
    }
    if (prev < in.length())
        out->push_back(in.substr(prev, string::npos));
}

int buildTable_v2(vector<string> *in) {
    string line = in->at(0);
    if (debug) cout << "target: " << line << endl;

    vector<string> wordVector;
    parseString(line, "{}:[]=,\"", &wordVector);
/*    size_t prev = 0, pos;
    while ((pos = line.find_first_of("{}:[]=,\"", prev)) != string::npos)
    {
        if (pos > prev)
            wordVector.push_back(line.substr(prev, pos-prev));
        prev = pos+1;
    }
    if (prev < line.length())
        wordVector.push_back(line.substr(prev, string::npos));
*/
    for (int i=0;i<wordVector.size()-1;) {
        if (debug) cout << wordVector.at(i) << endl;
        string token = wordVector.at(i++);
        if (!token.compare("Etas")) {
            while (!wordVector.at(i).compare("idx")) {
                i++;
                int index = 0;
                busstop_unit bu;
                std::istringstream (wordVector.at(i++)) >> index;
                if (debug) cout << index << " -> ";
                bu.index = index;
                string etas = wordVector.at(i++);
                if (etas.compare("eta"))
                    cerr << "format error" << endl;
                int eta;
                std::istringstream (wordVector.at(i)) >> eta;
                if (debug) cout << eta << endl;
                bu.eta = eta;
                v_busstop.push_back(bu);
                if (i < wordVector.size()-1)
                    i++;
            }
        } else if (!token.compare("Buses")) {
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
                if (debug) cout << plate << " is at index: " << index << endl;
                if (i >= wordVector.size()-1)
                    break;
                while (wordVector.at(++i).compare("bn")) {
                    if (i >= wordVector.size()-1)
                        break;
                }
            }
        }
    }

    return 0;
}

int storeToDB() {
    Connection con(false);
    int rs = con.connect("gbus", "localhost", "gbus", "gbus");
    if (debug) cout << "connect to DB: rs = " << rs << endl;

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
    if (debug) cout << "rows = " << sr.rows() << endl;
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
    if (debug) cout << "rows = " << sr.rows() << endl;

    return 0;
}

int main(int argc, char* argv[]) {
    vector<string> DataArray;
    debug = false;
    int opt;
    string filename;
    bool notStoreToDB = false;

    while ((opt = getopt(argc, argv, "i:db")) != -1) {
        switch (opt) {
            case 'i':
                filename = optarg;
                break;
            case 'd':
                debug = true;
                break;
            case 'b':
                notStoreToDB = true;
                break;
            default:
                cout << "unknown option" << endl;
        }
    }

    if (filename.length() != 0) {
        ifstream myfile(filename.c_str());

        if (!myfile) {
            cout << "Error opening output file: " << filename << endl;
            return -1;
        }

        copy(istream_iterator<string>(myfile), istream_iterator<string>(), back_inserter(DataArray));
        myfile.close();

        vector<string> wordVector;
        parseString(filename, "_/.-", &wordVector);

        busname = wordVector.at(1);
        key.append(busname);
        key.append("-");
        if (debug) cout << busname << endl;
        for (int i=2;i<wordVector.size();i++){
            date[i-2] = (wordVector.at(i)[0] - '0')*10 + (wordVector.at(i)[1] - '0');
        }

        wordVector.clear();

        if (DataArray.size() > 1) {
            for (int i = 1;i<DataArray.size();i++) {
                DataArray.at(0).append(" ");
                DataArray.at(0).append(DataArray.at(i));
            }
            if (debug) cout << "after: ->" << DataArray.at(0) << "<-" << endl;
        }

        int rs = buildTable_v2(&DataArray);

        cout << "key-> " << key;
        cout << " [ " << v_busstop.size();
        cout << " / " << v_buswhere.size() << "]" << endl;
        total_stop = v_busstop.size();
        total_bus = v_buswhere.size();

        if (!notStoreToDB)
            rs = storeToDB(/*&busData*/);

        DataArray.clear();
    }
    return 0;
}
