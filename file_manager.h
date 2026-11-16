#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include <string>
#include <vector>
#include "models.h"

class FileManager {
    std::string productFile, customerFile, orderFile, adminFile;
    mutable bool productsBlocked = false, customersBlocked = false, ordersBlocked = false;
    bool saveText(const std::string& path, const std::string& data) const;
    bool loadText(const std::string& path, std::string& data, bool& exists) const;
public:
    FileManager();
    const std::string& getProductFile() const;
    const std::string& getCustomerFile() const;
    const std::string& getOrderFile() const;
    bool saveProducts(const std::vector<Product>&) const;
    bool loadProducts(std::vector<Product>&) const;
    bool saveCustomers(const std::vector<Customer>&) const;
    bool loadCustomers(std::vector<Customer>&) const;
    bool saveOrders(const std::vector<Order>&) const;
    bool loadOrders(std::vector<Order>&) const;
    bool adminExists() const;
    bool saveAdmin(const std::string& username, const std::string& passwordRecord) const;
    bool verifyAdmin(const std::string& username, const std::string& password) const;
};
#endif
