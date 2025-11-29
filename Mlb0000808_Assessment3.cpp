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

using namespace std;

// ===================== SHA-256 Password Hashing =====================
// Simplified hashing function (placeholder for real SHA-256)
string sha256(const string& msg){
    unsigned long hash=0;                // Simple hash accumulator
    for(char c:msg) hash=(hash*101 + c)%1000000007; // Basic hashing mechanism
    return to_string(hash);              // Return hash as string
}

// ===================== UTILITY FUNCTIONS =====================
// Trim whitespace from both ends of a string
string trim(const string &s){
    size_t a=s.find_first_not_of(" \t\r\n");  
    if(a==string::npos) return "";       
    size_t b=s.find_last_not_of(" \t\r\n");
    return s.substr(a,b-a+1);           
}

// Validate numeric amount
bool is_valid_amount(const string& s){
    if(s.empty()) return false;          
    bool dot=false; size_t start=0;      
    if(s[0]=='-') start=1;               
    for(size_t i=start;i<s.size();i++){
        if(s[i]=='.'){ if(dot) return false; dot=true; } 
        else if(!isdigit(s[i])) return false;           
    }
    try{ stod(s); return true; } catch(...){ return false; } 
}

// Validate date format YYYY-MM-DD
bool is_valid_date(const string& d){
    if(d.size()!=10||d[4]!='-'||d[7]!='-') return false;    
    for(int i=0;i<10;i++){ if(i==4||i==7) continue; if(!isdigit(d[i])) return false; }
    int y=stoi(d.substr(0,4)), m=stoi(d.substr(5,2)), day=stoi(d.substr(8,2));
    return y>=1900&&y<=2100&&m>=1&&m<=12&&day>=1&&day<=31;  
}

// Remove commas to keep CSV stable
string sanitize(const string& s){
    string r; for(char c:s) r+=(c!=','?c:' '); return trim(r); 
}

// ===================== TRANSACTION STRUCTURE =====================
// Node structure for linked list
struct Transaction{
    string id,date,category,desc,type;  // Data fields
    double amount;                      // Amount field
    Transaction* next;                  // Pointer to next node
    Transaction() : id(""), date(""), category(""), desc(""), amount(0.0), type(""), next(nullptr) {}
    Transaction(string i,string d,string c,string ds,double a,string t)
        : id(i), date(d), category(c), desc(ds), amount(a), type(t), next(nullptr) {}
};

// ===================== LINKED LIST =====================
// Manage transaction nodes in a linked list
class TransactionList{
    Transaction* head;                  // Head pointer
    size_t sz;                          // Track list size
public:
    TransactionList(): head(nullptr), sz(0) {}
    ~TransactionList(){ while(head){ Transaction* t=head; head=head->next; delete t; } }
    void push_front(Transaction* t){ t->next=head; head=t; sz++; }
    bool remove_by_id(const string& id){
        Transaction *cur=head, *prev=nullptr;
        while(cur){
            if(cur->id==id){
                if(prev) prev->next=cur->next; else head=cur->next;
                delete cur; sz--; return true;
            }
            prev=cur; cur=cur->next;
        } return false;
    }
    Transaction* find_by_id(const string& id){
        Transaction* cur=head; while(cur){ if(cur->id==id) return cur; cur=cur->next; } return nullptr;
    }
    Transaction* get_head(){ return head; }
    size_t size() const { return sz; }
};

// ===================== HASH MAP =====================
// Efficient searching using map
class TransactionHashMap{
    map<string,Transaction*> idMap;                     // Map ID → Transaction
    map<string,vector<Transaction*>> categoryMap;       // Category indexing
public:
    void add(Transaction* t){ idMap[t->id]=t; categoryMap[t->category].push_back(t); }
    void remove(const string& id){
        auto it=idMap.find(id);
        if(it!=idMap.end()){
            Transaction* t=it->second;
            auto& vec=categoryMap[t->category];
            vec.erase(std::remove(vec.begin(),vec.end(),t),vec.end());
            idMap.erase(it);
        }
    }
    Transaction* find_by_id(const string& id){ return idMap.count(id)?idMap[id]:nullptr; }
    vector<Transaction*> find_by_category(const string& cat){ return categoryMap.count(cat)?categoryMap[cat]:vector<Transaction*>(); }
    void clear(){ idMap.clear(); categoryMap.clear(); }
};

// ===================== RECENT QUEUE =====================
// Maintain last 10 inserted transactions
class RecentTransactionsQueue{
    queue<Transaction*> recent; 
    static const size_t MAX_SIZE=10;
public:
    void add(Transaction* t){ if(recent.size()>=MAX_SIZE) recent.pop(); recent.push(t); }
    void display(){
        if(recent.empty()){ cout<<"No recent transactions.\n"; return; }
        queue<Transaction*> temp=recent; int c=1;
        cout<<"\n=== Recent Transactions ===\n";
        while(!temp.empty()){ Transaction* t=temp.front(); temp.pop();
            cout<<c++<<". ["<<t->id<<"] "<<t->date<<" | "<<t->category<<" | "<<t->desc<<" | $"<<fixed<<setprecision(2)<<t->amount<<" ("<<t->type<<")\n";
        }
    }
    void clear(){ while(!recent.empty()) recent.pop(); }
};

// ===================== UNDO STACK =====================
// Store last 20 deleted transactions for undo
class UndoStack{
    stack<Transaction> st; 
    static const size_t MAX_SIZE=20;
public:
    void push(const Transaction& t){ if(st.size()<MAX_SIZE) st.push(t); }
    bool pop(Transaction& t){ if(st.empty()) return false; t=st.top(); st.pop(); return true; }
    bool empty(){ return st.empty(); }
};

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

// ===================== HELPER FUNCTIONS =====================
// Generate unique transaction ID
string gen_id(){ return "TX"+to_string(time(nullptr))+to_string(rand()%1000); }

// Get today's date
string current_date(){ 
    time_t t=time(nullptr); 
    tm* tm=localtime(&t); 
    char buf[11]; 
    sprintf(buf,"%04d-%02d-%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday); 
    return string(buf); 
}

// Print table header
void print_header(){ 
    cout<<"\n-------------------------------------------------------------------------------------------------\n| ID                | Date        | Category       | Description         | Amount     | Type    |\n-------------------------------------------------------------------------------------------------\n"; 
}

// Print one transaction row
void print_transaction(Transaction* t){ 
    cout<<"| "<<setw(18)<<left<<t->id<<"| "<<setw(12)<<t->date<<"| "<<setw(15)<<t->category<<"| "<<setw(20)<<t->desc.substr(0,18)<<"| $"<<setw(10)<<right<<fixed<<setprecision(2)<<t->amount<<"| "<<setw(8)<<t->type<<"|\n"; 
}

// Print footer line
void print_footer(){ cout<<"-------------------------------------------------------------------------------------------------\n"; }

// ===================== INTERACTIVE MENU =====================
// Main menu for user operations
void menu(TransactionList& list,TransactionHashMap& hm,RecentTransactionsQueue& recent,UndoStack& undo,map<string,User>& users,string role){
    while(true){
        cout<<"\n1.Add 2.View 3.Delete 4.Search 5.Recent 6.Undo 7.Exit\nChoice: "; 
        string ch; getline(cin,ch);

        if(ch=="1"){  // Add Transaction
            string cat,desc,type,amtStr; 
            double amt; 
            string date=current_date();    
            cout<<"Category: "; getline(cin,cat); cat=sanitize(cat);
            cout<<"Description: "; getline(cin,desc); desc=sanitize(desc);
            cout<<"Amount: "; getline(cin,amtStr); 
            if(!is_valid_amount(amtStr)){ cout<<"Invalid amount\n"; continue; }
            amt=stod(amtStr);
            cout<<"Type (Income/Expense): "; getline(cin,type); 
            if(type!="Income"&&type!="Expense"){ cout<<"Invalid type\n"; continue; }
            Transaction* t=new Transaction(gen_id(),date,cat,desc,amt,type);
            list.push_front(t); hm.add(t); recent.add(t); undo.push(*t); 
            save_transactions(list);
            cout<<"Added.\n";

        }else if(ch=="2"){  // View all transactions
            if(list.size()==0){ cout<<"No transactions.\n"; continue; }
            print_header(); 
            Transaction* cur=list.get_head(); 
            while(cur){ print_transaction(cur); cur=cur->next;} 
            print_footer();

        }else if(ch=="3"){  // Delete transaction
            if(role!="admin"){ cout<<"Only admin can delete.\n"; continue; }
            cout<<"ID to delete: "; string id; getline(cin,id); id=sanitize(id);
            Transaction* t=list.find_by_id(id); 
            if(!t){ cout<<"Not found.\n"; continue; }
            undo.push(*t); hm.remove(id); list.remove_by_id(id); save_transactions(list); 
            cout<<"Deleted.\n";

        }else if(ch=="4"){  // Search by category
            cout<<"Category: "; string cat; getline(cin,cat); cat=sanitize(cat);
            auto vec=hm.find_by_category(cat); 
            if(vec.empty()){ cout<<"No transactions.\n"; continue; }
            print_header(); 
            for(auto t:vec) print_transaction(t);

        }else if(ch=="5"){ recent.display(); } // Show recent

        else if(ch=="6"){  // Undo last remove
            Transaction t; 
            if(!undo.pop(t)){ cout<<"Nothing to undo.\n"; continue; }
            Transaction* nt=new Transaction(t.id,t.date,t.category,t.desc,t.amount,t.type);
            list.push_front(nt); hm.add(nt); recent.add(nt); save_transactions(list); 
            cout<<"Undo restored.\n";

        }else if(ch=="7"){ cout<<"Exiting...\n"; break; }

        else{ cout<<"Invalid choice.\n"; }
    }
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
