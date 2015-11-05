#include <unistd.h>
#include <chrono>
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
#include <sstream>
#include <mysql++.h>
using mysqlpp::Connection;
using mysqlpp::Query;
using mysqlpp::StoreQueryResult;
using mysqlpp::SimpleResult;
using mysqlpp::Row;

string key;
string curtime;
string busname;
int date[5] = {0}; //month, day, hour, min, sec
int total_bus;
int total_stop;

bool debug;
bool recordtime;

struct busstop_unit {
    int index;
    int eta;
};

struct bus_where {
    string plate;
    int where;
    bool in;
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
    auto t_start = std::chrono::high_resolution_clock::now();
    string line = in->at(0);
    if (debug) cout << "target: " << line << endl;

    vector<string> wordVector;
    parseString(line, "{}:[]=,\"", &wordVector);

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
                    cout << "format error" << endl;
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
                } else cout << "format error" << endl;
                string idx = wordVector.at(++i);
                if (idx.compare("idx"))
                    cout << "format error" << endl;
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

    auto t_end = std::chrono::high_resolution_clock::now();
    if (recordtime) cout << "buildtable takes: " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << " ms" << endl;
    return 0;
}

StoreQueryResult selectFromDb(Query query, string tablename, vector<string> keys, vector<string> keyvalues, vector<string> columns) {
    auto t_start = std::chrono::high_resolution_clock::now();
    StoreQueryResult res;
    string select("SELECT ");
    if (columns.size() > 0) {
        for (int i=0;i<columns.size();i++) {
            select.append(columns.at(i));
            if (i!=columns.size()-1)
                select.append(",");
        }
    } else
        select.append("*");
    select.append(" FROM ");
    select.append(tablename);
    if (keys.size() != 0 && keyvalues.size() != 0) {
        select.append(" WHERE ");
        for (int i=0;i<keys.size();i++) {
            select.append(keys.at(i));
            select.append("='");
            select.append(keyvalues.at(i));
            select.append("'");
            if (i!=keys.size()-1)
                select.append(" AND ");
        }
    }
    if (debug) cout << "select: " << select << endl;
    query << select;

    try {
        res = query.store();
    } catch (const mysqlpp::BadQuery& er) {
        cout << "Query error: " << er.what() << endl;
    }catch (const mysqlpp::BadConversion& er) {
        cout << "Conversion error: " << er.what() << endl;
        cout << "retrieved data size: " << er.retrieved << ", actual size: " << er.actual_size << endl;
    }catch (const mysqlpp::Exception& er) {
        cout << "Error: " << er.what() << endl;
    }

    query.reset();
    auto t_end = std::chrono::high_resolution_clock::now();
    if (recordtime) cout << "selectFromDb takes " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << " ms" << endl;
    return res;
}

int insertDataToDb(Query query, string tablename, vector<string> columns, vector<string> values) {
    auto t_start = std::chrono::high_resolution_clock::now();
    int ret = 0;
    if (columns.size() != values.size()) {
        cout << "columns.size() != values.size()" << endl;
        return ret;
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
        if (sr.rows()==0) {
            cout << "error: " << query.error() << endl;
        } else {
            if (debug) cout << "rows = " << sr.rows() << " id = " << sr.insert_id() << " info: " << sr.info() << endl;
            ret = 1;
        }
    } catch (const mysqlpp::BadQuery& er) {
        cout << "Query error: " << er.what() << endl;
    }catch (const mysqlpp::BadConversion& er) {
        cout << "Conversion error: " << er.what() << endl;
        cout << "retrieved data size: " << er.retrieved << ", actual size: " << er.actual_size << endl;
    }catch (const mysqlpp::Exception& er) {
        cout << "Error: " << er.what() << endl;
    }

    query.reset();
    auto t_end = std::chrono::high_resolution_clock::now();
    if (recordtime) cout << "insertDataToDb takes " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << " ms" << endl;
    return ret;
}

int updateDataToDb(Query query, string tablename, string key, string keyvalue, vector<string> columns, vector<string> values) {
    auto t_start = std::chrono::high_resolution_clock::now();
    int ret = 0;
    string update;
    update.append("UPDATE ");
    update.append(tablename);
    update.append(" SET ");

    if (columns.size() != values.size()) {
        cout << "updateDataToDb: size does not match: " << columns.size() << " vs " << values.size() << endl;
        return ret;
    }

    for (int i=0;i<columns.size();i++) {
        update.append(columns.at(i));
        update.append("='");
        update.append(values.at(i));
        update.append("'");
        if (i!=columns.size()-1)
            update.append(",");
    }

    update.append(" WHERE ");
    update.append(key);
    update.append("='");
    update.append(keyvalue);
    update.append("'");

    if (debug) cout << "update: " << update << endl;
    query << update;

    try {
        SimpleResult sr = query.execute();
        if (sr.rows()==0) {
            cout << "error: " << query.error() << endl;
        } else {
            if (debug) cout << "rows = " << sr.rows() << " id = " << sr.insert_id() << " info: " << sr.info() << endl;
            ret = 1;
        }
    } catch (const mysqlpp::BadQuery& er) {
        cout << "Query error: " << er.what() << endl;
    }catch (const mysqlpp::BadConversion& er) {
        cout << "Conversion error: " << er.what() << endl;
        cout << "retrieved data size: " << er.retrieved << ", actual size: " << er.actual_size << endl;
    }catch (const mysqlpp::Exception& er) {
        cout << "Error: " << er.what() << endl;
    }

    query.reset();
    auto t_end = std::chrono::high_resolution_clock::now();
    if (recordtime) cout << "updateDataToDb takes " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << " ms" << endl;
    return ret;
}

int storeToDB() {
    auto t_start = std::chrono::high_resolution_clock::now();
    Connection con(false);
    int rs = con.connect("gbus", "localhost", "gbus", "gbus");
    if (debug) cout << "connect to DB: rs = " << rs << endl;
    if (rs != 1)
        return rs;

    Query query = con.query();

    vector<string> columns;
    vector<string> values;
#if 0
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

    columns.push_back("total_bus");
    values.push_back(int2str(total_bus));

    if (total_bus > 0) {
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
    }

    columns.push_back("bus_name");
    values.push_back(busname);
    insertDataToDb(query, "route", columns, values);
#endif
    columns.clear();
    values.clear();

    // store to businfo
    vector<string> keys;
    vector<string> keyvalues;
    if (total_bus > 0){
        for (int i=0;i<=total_bus-1;i++) {
            string curbusplate(v_buswhere.at(i).plate);
            int curbuswhere = v_buswhere.at(i).where;
            int curbusin = v_buswhere.at(i).in;

            if (curbuswhere==0) {
                if (curbusin)  { // insert
                    keys.push_back("bus_plate");
                    keyvalues.push_back(curbusplate);
                    keys.push_back("complete");
                    keyvalues.push_back("0");
                    keys.push_back("bus_name");
                    keyvalues.push_back(busname);
                    columns.push_back("bus_key");
                    columns.push_back("stoptime_0");
                    columns.push_back("started");
                    StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                    columns.clear();
                    keys.clear();
                    keyvalues.clear();
                    if (res.size()==0) {
                        columns.push_back("bus_key");
                        values.push_back(key);
                        columns.push_back("bus_plate");
                        values.push_back(curbusplate);
                        columns.push_back("stoptime_0");
                        values.push_back(curtime);
                        columns.push_back("bus_name");
                        values.push_back(busname);
                        columns.push_back("totalstop");
                        values.push_back(int2str(total_stop));
                        if (!insertDataToDb(query, "businfo", columns, values))
                            cout << "something wrong..." << endl;
                        columns.clear();
                        values.clear();
                    }
                } else { //update if started = 0, and set started = 1 after updated
                    keys.push_back("bus_plate");
                    keyvalues.push_back(curbusplate);
                    keys.push_back("complete");
                    keyvalues.push_back("0");
                    keys.push_back("bus_name");
                    keyvalues.push_back(busname);
                    columns.push_back("bus_key");
                    columns.push_back("stoptime_0");
                    columns.push_back("started");
                    StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                    columns.clear();
                    keys.clear();
                    keyvalues.clear();
                    if (res.size()==0) {
                        if (debug) cout << "insert bus data since not found" << endl;
                        columns.push_back("bus_key");
                        values.push_back(key);
                        columns.push_back("bus_plate");
                        values.push_back(curbusplate);
                        columns.push_back("stoptime_0");
                        values.push_back(curtime);
                        columns.push_back("bus_name");
                        values.push_back(busname);
                        columns.push_back("totalstop");
                        values.push_back(int2str(total_stop));
                        columns.push_back("started");
                        values.push_back("1");
                        if (!insertDataToDb(query, "businfo", columns, values))
                            cout << "something wrong..." << endl;
                        columns.clear();
                        values.clear();
                    } else if (res.size()==1) {
                        Row row = res.at(0);
                        if (!row.at(2).compare("0")) {
                            columns.push_back("started");
                            values.push_back("1");
                            columns.push_back("stoptime_0");
                            values.push_back(curtime);
                            updateDataToDb(query, "businfo", "bus_key", row.at(0).c_str(), columns, values);
                            columns.clear();
                            values.clear();
                        }
                    } else {
                        cout << "something wrong... more than 1 bus is not complete with same plate" <<endl;
                    }
                }
/*            } else if (curbuswhere==1) {
                keys.push_back("bus_plate");
                keyvalues.push_back(curbusplate);
                keys.push_back("complete");
                keyvalues.push_back("0");
                keys.push_back("bus_name");
                keyvalues.push_back(busname);
                columns.push_back("bus_key");
                columns.push_back("stoptime_0");
                columns.push_back("started");
                StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                columns.clear();
                keys.clear();
                keyvalues.clear();
                if (res.size()==0) {
                    if (debug) cout << "insert bus data since not found" << endl;
                    columns.push_back("bus_key");
                    values.push_back(key);
                    columns.push_back("bus_plate");
                    values.push_back(curbusplate);
                    columns.push_back("stoptime_0");
                    values.push_back(curtime);
                    columns.push_back("stoptime_1");
                    values.push_back(curtime);
                    columns.push_back("bus_name");
                    values.push_back(busname);
                    columns.push_back("totalstop");
                    values.push_back(int2str(total_stop));
                    columns.push_back("started");
                    values.push_back("1");
                    if (!insertDataToDb(query, "businfo", columns, values))
                        cout << "something wrong..." << endl;
                    columns.clear();
                    values.clear();
                }*/
            } else if (curbuswhere==total_stop-1) {
                if (curbusin) { // update where and set complete = 1 if complete != 1
                    keys.push_back("bus_plate");
                    keyvalues.push_back(curbusplate);
                    keys.push_back("bus_name");
                    keyvalues.push_back(busname);
                    columns.push_back("bus_key");
                    string s("stoptime_");
                    s.append(int2str(curbuswhere));
                    columns.push_back(s);
                    columns.push_back("complete");
                    StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                    columns.clear();
                    keys.clear();
                    keyvalues.clear();
                    if (res.size()==1) {
                        Row row = res.at(0);
                        if (row.at(2).compare("1")) {
                            columns.push_back(s);
                            values.push_back(curtime);
                            columns.push_back("complete");
                            values.push_back("1");
                            updateDataToDb(query, "businfo", "bus_key", row.at(0).c_str(), columns, values);
                            columns.clear();
                            values.clear();
                        }
                    } else {
                        cout << "something wrong... found 0 or 2+ buses to complete " << res.size() << endl;
                    }
                } else { // X
                    cout << "something wrong... total_stop is wrong? " << curbuswhere << " vs " << total_stop << endl;
                }
            } else if (curbuswhere==total_stop-2) {
                if (curbusin) { // update where
                    keys.push_back("bus_plate");
                    keyvalues.push_back(curbusplate);
                    keys.push_back("complete");
                    keyvalues.push_back("0");
                    keys.push_back("bus_name");
                    keyvalues.push_back(busname);
                    columns.push_back("bus_key");
                    string s("stoptime_");
                    s.append(int2str(curbuswhere));
                    columns.push_back(s);
                    StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                    columns.clear();
                    keys.clear();
                    keyvalues.clear();
                    if (res.size()==1) {
                        Row row = res.at(0);
                        if (!row.at(1).compare("NULL")) {
                            columns.push_back(s);
                            values.push_back(curtime);
                            updateDataToDb(query, "businfo", "bus_key", row.at(0).c_str(), columns, values);
                            columns.clear();
                            values.clear();
                        }
                    } else {
                        cout << "something wrong... found 0 or 2+ buses to complete " << res.size() << endl;
                    }
                } else { //update complete = 2, and where with stoptime = cur + eta
                    keys.push_back("bus_plate");
                    keyvalues.push_back(curbusplate);
                    keys.push_back("complete");
                    keyvalues.push_back("0");
                    keys.push_back("bus_name");
                    keyvalues.push_back(busname);
                    columns.push_back("bus_key");
                    string s("stoptime_");
                    s.append(int2str(curbuswhere));
                    columns.push_back(s);
                    StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                    columns.clear();
                    keys.clear();
                    keyvalues.clear();
                    if (res.size()==1) {
                        Row row = res.at(0);
                        if (!row.at(1).compare("NULL")) {
                            int nextwhere = curbuswhere+1;
                            s.clear();
                            s.append("stoptime_");
                            s.append(int2str(nextwhere));
                            columns.push_back(s);
                            string nexttime = curtime;
                            int eta = v_busstop[nextwhere].eta;
                            values.push_back(nexttime);
                            columns.push_back("complete");
                            values.push_back("2");
                            updateDataToDb(query, "businfo", "bus_key", row.at(0).c_str(), columns, values);
                            columns.clear();
                            values.clear();
                        }
                    } else {
                        cout << "something wrong... found 0 or 2+ buses to complete " << res.size() << endl;
                    }
                }
            } else {
                keys.push_back("bus_plate");
                keyvalues.push_back(curbusplate);
                keys.push_back("complete");
                keyvalues.push_back("0");
                keys.push_back("bus_name");
                keyvalues.push_back(busname);
                columns.push_back("bus_key");
                string s("stoptime_");
                s.append(int2str(curbuswhere));
                columns.push_back(s);
                columns.push_back("started");
                StoreQueryResult res = selectFromDb(query, "businfo", keys, keyvalues, columns);
                columns.clear();
                keys.clear();
                keyvalues.clear();
                if (res.size()==1) {
                    Row row = res.at(0);
                    if (!row.at(1).compare("NULL")) {
                        columns.push_back(s);
                        values.push_back(curtime);
                        if (!row.at(2).compare("0")) {
                            columns.push_back("started");
                            values.push_back("1");
                        }
                        updateDataToDb(query, "businfo", "bus_key", row.at(0).c_str(), columns, values);
                        columns.clear();
                        values.clear();
                    }
                } else {
                    cout << "something wrong... found 0 or 2+ buses to complete " << res.size() << endl;
                }
            }
        }
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    if (recordtime) cout << "storeToDb takes: " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << " ms" << endl;
    return 0;
}

int main(int argc, char* argv[]) {
    auto t_start = std::chrono::high_resolution_clock::now();
    debug = false;
    recordtime = false;
    int opt;
    string filename;
    bool notStoreToDB = false;

    while ((opt = getopt(argc, argv, "i:dbt")) != -1) {
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
            case 't':
                recordtime = true;
                break;
            default:
                cout << "unknown option" << endl;
        }
    }

    if (filename.length() != 0) {
        vector<string> DataArray;
        ifstream myfile(filename.c_str());

        if (!myfile) {
            cout << "Error opening output file: " << filename << endl;
            return -1;
        }

        copy(istream_iterator<string>(myfile), istream_iterator<string>(), back_inserter(DataArray));
        myfile.close();

        if (DataArray.size() == 0) {
            cout << "file size is 0" << endl;
            return -1;
        }

        vector<string> wordVector;
        parseString(filename, "_/", &wordVector);

        busname = wordVector.at(1);
        key.append(busname);
        if (debug) cout << busname << endl;
        for (int i=2;i<wordVector.size();i++){
            key.append("-");
            date[i-2] = (wordVector.at(i)[0] - '0')*10 + (wordVector.at(i)[1] - '0');
            if (date[i-2] < 10) {
                key.append("0");
                curtime.append("0");
            }
            key.append(int2str(date[i-2]));
            curtime.append(int2str(date[i-2]));
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
    auto t_end = std::chrono::high_resolution_clock::now();
    if (recordtime) cout << "total takes: " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << " ms" << endl;
    return 0;
}
