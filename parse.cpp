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
    bool in;
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
                i = i+4;
                string io = wordVector.at(i);
                if (!io.compare("i")) {
                    bw.in = true;
                } else if (!io.compare("o")) {
                    bw.in = false;
                } else cerr << "format error" << endl;
                string idx = wordVector.at(++i);
                if (idx.compare("idx"))
                    cerr << "format error" << endl;
                int index = 0;
                std::istringstream (wordVector.at(++i)) >> index;
                bw.where = index;
                if (debug) cout << bw.plate << (bw.in ? " is just at index: " : " is leaving index ") << index << endl;
                v_buswhere.push_back(bw);
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

StoreQueryResult selectFromDb(Query query, string tablename, vector<string> columns) {
}

int insertDataToDb(Query query, string tablename, vector<string> columns, vector<string> values) {
    if (columns.size() != values.size()) {
        cerr << "columns.size() != values.size()" << endl;
        return 0;
    }
    string insert;
    insert.append("INSERT INTO ");
    insert.append(tablename);
    insert.append(" (");
    for (int i=0;i<columns.size();i++) {
        insert.append(columns.at(i));
        if (i!=columns.size()-1)
            insert.append(",");
    }
    insert.append(")");

    insert.append(" VALUES ");
    insert.append("(");
    for (int i=0;i<values.size();i++) {
        insert.append("'");
        insert.append(values.at(i));
        insert.append("'");
        if (i!=values.size()-1)
            insert.append(",");
    }
    insert.append(")");

    if (debug) cout << "insert: " << insert << endl;
    query << insert;

    try {
        SimpleResult sr = query.execute();
        if (debug) {
            if (sr.rows()==0) {
                cout << "error: " << query.error() << endl;
            } else
                cout << "rows = " << sr.rows() << " id = " << sr.insert_id() << " info: " << sr.info() << endl;
        }
    } catch (const mysqlpp::BadQuery& er) {
        cerr << "Query error: " << er.what() << endl;
    }catch (const mysqlpp::BadConversion& er) {
        cerr << "Conversion error: " << er.what() << endl;
        cerr << "\tretrieved data size: " << er.retrieved << ", actual size: " << er.actual_size << endl;
    }catch (const mysqlpp::Exception& er) {
        cerr << "Error: " << er.what() << endl;
    }

    query.reset();
    return 0;
}

int updateDataToDb(Query query, string tablename, string key, vector<string> columns, vector<string> values, bool simple) {
    string update;
}

int storeToDB() {
    Connection con(false);
    int rs = con.connect("gbus", "localhost", "gbus", "gbus");
    if (debug) cout << "connect to DB: rs = " << rs << endl;
    if (rs != 1)
        return rs;

    Query query = con.query();

    vector<string> columns;
    vector<string> values;
    columns.push_back("bus_storetime");
    values.push_back(key);
    columns.push_back("month");
    columns.push_back("day");
    columns.push_back("hour");
    columns.push_back("min");
    columns.push_back("sec");
    for (int i=0;i<5;i++) {
        values.push_back(int2str(date[i]));
    }

    columns.push_back("total_stop");
    values.push_back(int2str(total_stop));

    for (int i=1;i<=total_stop;i++) {
        string s("stopstatus_");
        s.append(int2str(i));
        columns.push_back(s);
        values.push_back(int2str(v_busstop[i-1].eta));
    }

    for (int i=1;i<=total_bus;i++) {
        string s("busstatus_");
        s.append(int2str(i));
        columns.push_back(s);
        values.push_back(int2str(v_buswhere[i-1].where));

        s.clear();
        s.append("busplate_");
        s.append(int2str(i));
        columns.push_back(s);
        values.push_back(v_buswhere[i-1].plate);
    }

    columns.push_back("bus_name");
    values.push_back(busname);

    insertDataToDb(query, "route", columns, values);
    columns.clear();
    values.clear();

    // store to businfo
    query.reset();
    query << "SELECT bus_plate,complete FROM businfo";
    StoreQueryResult res = query.store();
    if (res && res.size() > 0) {
        if(debug) cout << res.size() << endl;
        Row row;
        Row::size_type i;
        for (i = 0; row = res.at(i); i++) {
            cout << "a " << row.at(0) << " " << row.at(1) << endl;
            if (i==res.size()-1) break;
        }
    } else {
        // no found any record, just insert a new one
        for (int i=0;i<v_buswhere.size();i++) {
            if (v_buswhere.at(i).where == 0 && v_buswhere.at(i).in == true) {
                // bus start at first stop
                columns.push_back("busname");
                columns.push_back("bus_plate");
                columns.push_back("stoptime_1");
                values.push_back(key);
                values.push_back(v_buswhere.at(i).plate);
                string s;
                for (int i=0;i<5;i++) {
                    s.append(int2str(date[i]));
                }
                values.push_back(s);
                insertDataToDb(query, "businfo", columns, values);
            }
        }
    }
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
        if (debug) cout << busname << endl;
        for (int i=2;i<wordVector.size();i++){
            key.append("-");
            date[i-2] = (wordVector.at(i)[0] - '0')*10 + (wordVector.at(i)[1] - '0');
            key.append(int2str(date[i-2]));
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

        if (!notStoreToDB) {
            rs = storeToDB();
        }

        DataArray.clear();
    }
    return 0;
}
