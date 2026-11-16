#include "file_manager.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <regex>
#include <set>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace {
void ensureDataDir() {
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0755);
#endif
}
bool replaceFile(const std::string& temp, const std::string& target) {
    std::remove((target + ".bak").c_str());
    std::ifstream old(target, std::ios::binary);
    if (old.good()) { old.close(); if (std::rename(target.c_str(), (target + ".bak").c_str()) != 0) return false; }
    if (std::rename(temp.c_str(), target.c_str()) != 0) {
        std::ifstream backup(target + ".bak", std::ios::binary);
        if (backup.good()) std::rename((target + ".bak").c_str(), target.c_str());
        return false;
    }
    return true;
}
}
FileManager::FileManager() : productFile("data/products.dat"), customerFile("data/customers.dat"), orderFile("data/orders.dat"), adminFile("data/admin.dat") { ensureDataDir(); }
const std::string& FileManager::getProductFile() const { return productFile; }
const std::string& FileManager::getCustomerFile() const { return customerFile; }
const std::string& FileManager::getOrderFile() const { return orderFile; }
bool FileManager::saveText(const std::string& path, const std::string& data) const {
    ensureDataDir(); const std::string tmp = path + ".tmp";
    { std::ofstream out(tmp, std::ios::binary | std::ios::trunc); if (!out) return false; out << data; out.flush(); if (!out.good()) return false; }
    return replaceFile(tmp, path);
}
bool FileManager::loadText(const std::string& path, std::string& data, bool& exists) const {
    std::ifstream in(path, std::ios::binary); exists = in.good(); if (!exists) { data.clear(); return true; }
    std::ostringstream ss; ss << in.rdbuf(); if (in.bad()) return false; data = ss.str(); return true;
}
bool FileManager::saveProducts(const std::vector<Product>& v) const {
    if(productsBlocked) return false;
    std::ostringstream o; o << "TECHSTORE_PRODUCTS 1 " << v.size() << '\n';
    for (auto& p:v) o << p.getId()<<' '<<std::quoted(p.getName())<<' '<<std::quoted(p.getCategory())<<' '<<p.getPricePaise()<<' '<<p.getStock()<<' '<<std::quoted(p.getDescription())<<'\n';
    return saveText(productFile,o.str());
}
bool FileManager::loadProducts(std::vector<Product>& v) const {
    std::string d; bool ex; if(!loadText(productFile,d,ex)||!ex) return !ex;
    std::istringstream in(d); std::string magic; int ver; size_t n; if(!(in>>magic>>ver>>n)||magic!="TECHSTORE_PRODUCTS"||ver!=1||n>100000) {productsBlocked=true; return false;}
    std::vector<Product> tmp; std::set<int> ids; for(size_t i=0;i<n;++i){int id,stock; long long paise; std::string name,cat,desc; if(!(in>>id>>std::quoted(name)>>std::quoted(cat)>>paise>>stock>>std::quoted(desc))||paise<=0||!ids.insert(id).second) {productsBlocked=true; return false;} try{tmp.emplace_back(id,name,cat,static_cast<double>(paise)/100.0,stock,desc);}catch(...){{productsBlocked=true; return false;}} }
    in>>std::ws; if(!in.eof()) {productsBlocked=true; return false;} v.swap(tmp); return true;
}
bool FileManager::saveCustomers(const std::vector<Customer>& v) const {
    if(customersBlocked) return false;
    std::ostringstream o; o << "TECHSTORE_CUSTOMERS 1 " << v.size() << '\n';
    for(auto& c:v) o<<c.getId()<<' '<<std::quoted(c.getName())<<' '<<std::quoted(c.getEmail())<<' '<<std::quoted(c.getPasswordRecord())<<' '<<std::quoted(c.getPhone())<<'\n';
    return saveText(customerFile,o.str());
}
bool FileManager::loadCustomers(std::vector<Customer>& v) const {
    std::string d; bool ex; if(!loadText(customerFile,d,ex)||!ex) return !ex;
    std::istringstream in(d); std::string magic; int ver; size_t n; if(!(in>>magic>>ver>>n)||magic!="TECHSTORE_CUSTOMERS"||ver!=1||n>100000){customersBlocked=true; return false;}
    std::vector<Customer> tmp; std::set<int> ids; std::set<std::string> emails; const std::regex recordPattern(R"(^pbkdf2_sha256\$[1-9][0-9]{0,7}\$[0-9a-fA-F]{32}\$[0-9a-fA-F]{64}$)");
    for(size_t i=0;i<n;++i){int id;std::string name,email,hash,phone;if(!(in>>id>>std::quoted(name)>>std::quoted(email)>>std::quoted(hash)>>std::quoted(phone))||id<=0||name.empty()||email.empty()||phone.empty()||!ids.insert(id).second||!emails.insert(email).second||!std::regex_match(hash,recordPattern)){customersBlocked=true; return false;} tmp.emplace_back(id,name,email,hash,phone);}
    in>>std::ws;if(!in.eof()){customersBlocked=true; return false;}v.swap(tmp);return true;
}
bool FileManager::saveOrders(const std::vector<Order>& v) const {
    if(ordersBlocked) return false;
    std::ostringstream o; o<<"TECHSTORE_ORDERS 1 "<<v.size()<<'\n';
    for(auto& x:v){o<<x.getOrderId()<<' '<<x.getCustomerId()<<' '<<std::quoted(x.getStatus())<<' '<<std::quoted(x.getOrderDate())<<' '<<x.getItems().size()<<'\n';for(auto& i:x.getItems())o<<i.getProductId()<<' '<<std::quoted(i.getProductName())<<' '<<i.getQuantity()<<' '<<i.getUnitPricePaise()<<'\n';}
    return saveText(orderFile,o.str());
}
bool FileManager::loadOrders(std::vector<Order>& v) const {
    std::string d;bool ex;if(!loadText(orderFile,d,ex)||!ex)return !ex;
    std::istringstream in(d);std::string magic;int ver;size_t n;if(!(in>>magic>>ver>>n)||magic!="TECHSTORE_ORDERS"||ver!=1||n>100000){ordersBlocked=true; return false;}
    std::vector<Order> tmp;std::set<int> ids;for(size_t k=0;k<n;++k){int id,cid;std::string status,date;size_t count;if(!(in>>id>>cid>>std::quoted(status)>>std::quoted(date)>>count)||id<=0||cid<=0||count==0||count>10000||!ids.insert(id).second||(status!="Pending"&&status!="Confirmed"&&status!="Shipped"&&status!="Delivered"&&status!="Cancelled")){ordersBlocked=true; return false;}Order x(id,cid);x.setStatus(status);x.restoreDate(date);for(size_t j=0;j<count;++j){int pid,q;long long paise;std::string name;if(!(in>>pid>>std::quoted(name)>>q>>paise)||pid<=0||q<=0||paise<=0){ordersBlocked=true; return false;}try{if(!x.addItem(OrderItem(pid,name,q,static_cast<double>(paise)/100.0))){ordersBlocked=true; return false;}}catch(...){{ordersBlocked=true; return false;}}}tmp.push_back(x);}
    in>>std::ws;if(!in.eof()){ordersBlocked=true; return false;}v.swap(tmp);return true;
}
bool FileManager::adminExists() const {std::ifstream f(adminFile);return f.good();}
bool FileManager::saveAdmin(const std::string& u,const std::string& h) const {std::ostringstream o;o<<"TECHSTORE_ADMIN 1\n"<<std::quoted(u)<<' '<<std::quoted(h)<<'\n';return saveText(adminFile,o.str());}
bool FileManager::verifyAdmin(const std::string& u,const std::string& p) const {std::ifstream in(adminFile);std::string m,name,hash;int v;if(!(in>>m>>v>>std::quoted(name)>>std::quoted(hash))||m!="TECHSTORE_ADMIN"||v!=1)return false;return u==name&&PasswordUtils::verifyPassword(p,hash);}
