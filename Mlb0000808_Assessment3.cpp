// Personal Finance Tracker – Advanced Enhancements
// StudentID: Mlb0000808

#include <iostream>      // For input/output
#include <fstream>       // For file handling
#include <sstream>       // For string stream operations
#include <string>        // For string usage
#include <vector>        // For dynamic array
#include <map>           // For map-based hash map
#include <queue>         // For queue (recent transactions)
#include <stack>         // For stack (undo functionality)
#include <ctime>         // For date/time
#include <iomanip>       // For formatting output
#include <algorithm>     // For remove/erase operations









// ===================== FILE HANDLING & ENCRYPTION =====================
// File names and encryption key
const string TX_FILE="transactions.csv"; 
const string USER_FILE="users.txt";
const string XOR_KEY="S3cureKey_v2!@#$";

// Simple XOR encryption/decryption
string xor_encrypt(const string& data,const string& key){
    string out=data; for(size_t i=0;i<data.size();i++) out[i]^=key[i%key.size()]; return out;
}

// Save all transactions to file
void save_transactions(TransactionList& list){
    try{
        ofstream ofs(TX_FILE, ios::binary|ios::trunc); 
        if(!ofs) throw runtime_error("Cannot open file!");
        stringstream ss; Transaction* cur=list.get_head();
        while(cur){
            ss<<cur->id<<","<<cur->date<<","<<sanitize(cur->category)<<","<<sanitize(cur->desc)<<","<<fixed<<setprecision(2)<<cur->amount<<","<<cur->type<<"\n";
            cur=cur->next;
        }
        ofs<<xor_encrypt(ss.str(),XOR_KEY); ofs.close();
    }catch(const exception& e){ cerr<<"Error saving: "<<e.what()<<"\n"; }
}

// Load transactions from file
void load_transactions(TransactionList& list,TransactionHashMap& hm,RecentTransactionsQueue& recent){
    try{
        ifstream ifs(TX_FILE, ios::binary); 
        if(!ifs){ cout<<"No transaction file found.\n"; return; }
        stringstream buffer; buffer<<ifs.rdbuf(); ifs.close();
        string data=xor_encrypt(buffer.str(),XOR_KEY); 
        istringstream ss(data); string line;
        while(getline(ss,line)){
            if(trim(line).empty()) continue;
            vector<string> parts; string tmp;
            for(char ch:line){ if(ch==','){ parts.push_back(tmp); tmp=""; } else tmp+=ch; } 
            parts.push_back(tmp);
            if(parts.size()!=6) continue;
            double amt=stod(parts[4]);
            Transaction* t=new Transaction(parts[0],parts[1],parts[2],parts[3],amt,parts[5]);
            list.push_front(t); hm.add(t); recent.add(t);
        }
    }catch(const exception& e){ cerr<<"Error loading: "<<e.what()<<"\n"; }
}


