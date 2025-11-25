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



// ===================== USER AUTHENTICATION =====================
// User struct for login management
struct User{ 
    string username,pw_hash,role; 
    User(string u="",string h="",string r="user"): username(u), pw_hash(h), role(r) {} 
};

// Load users from file
map<string,User> load_users(){
    map<string,User> users;
    try{
        ifstream ifs(USER_FILE); if(!ifs) return users;
        string line; while(getline(ifs,line)){ 
            if(trim(line).empty()) continue;
            vector<string> parts; string tmp;
            for(char c:line){ if(c==':'){ parts.push_back(tmp); tmp=""; } else tmp+=c; } 
            parts.push_back(tmp);
            if(parts.size()==3) users[parts[0]]=User(parts[0],parts[1],parts[2]);
        }
        ifs.close();
    }catch(...){ cerr<<"Error loading users\n"; }
    return users;
}

// Save user credentials
void save_users(const map<string,User>& users){
    try{ ofstream ofs(USER_FILE, ios::trunc); 
         for(auto& p:users) ofs<<p.second.username<<":"<<p.second.pw_hash<<":"<<p.second.role<<"\n"; 
    }catch(...){ cerr<<"Error saving users\n"; }
}

// Create admin user if missing
void ensure_admin(map<string,User>& users){
    if(!users.count("admin")){
        users["admin"]=User("admin",sha256("admin123"),"admin");
        save_users(users);
        cout<<"Default admin created (admin/admin123)\n";
    }
}

// Authenticate user login
bool authenticate(map<string,User>& users,string& outUser,string& outRole){
    cout<<"Username: "; string u; getline(cin,u);
    cout<<"Password: "; string p; getline(cin,p);
    if(!users.count(u)||users[u].pw_hash!=sha256(p)){ cout<<"Invalid login.\n"; return false; }
    outUser=u; outRole=users[u].role; return true;
}

// Register a new user
bool register_user(map<string,User>& users){
    cout<<"Username: "; string u; getline(cin,u); u=sanitize(u);
    if(u.length()<3||users.count(u)){ cout<<"Invalid or existing username.\n"; return false; }
    cout<<"Password: "; string p; getline(cin,p); 
    if(p.length()<6){ cout<<"Too short.\n"; return false; }
    users[u]=User(u,sha256(p),"user"); 
    save_users(users); 
    cout<<"Registration success!\n"; 
    return true;
}

// ===================== MAIN =====================
// Program entry point
int main(){
    srand(time(nullptr));                  // Seed randomness
    TransactionList list; 
    TransactionHashMap hm; 
    RecentTransactionsQueue recent; 
    UndoStack undo;

    map<string,User> users=load_users();   // Load users
    ensure_admin(users);                   // Ensure admin exists

    load_transactions(list,hm,recent);     // Load stored transactions

    cout<<"=== Personal Finance Tracker ===\n";

    while(true){
        cout<<"\n1.Login 2.Register 3.Exit\nChoice: "; 
        string ch; getline(cin,ch);

        if(ch=="1"){ 
            string u,r; 
            if(authenticate(users,u,r)) menu(list,hm,recent,undo,users,r); 
        }
        else if(ch=="2"){ register_user(users); }
        else if(ch=="3"){ cout<<"Goodbye.\n"; break; }
        else{ cout<<"Invalid choice.\n"; }
    }
    return 0;
}



