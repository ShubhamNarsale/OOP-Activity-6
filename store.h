// store.h - The Store class: owns the data collections and runs the menus
#ifndef STORE_H
#define STORE_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include "models.h"
#include "file_manager.h"

class Store {
private:
    std::vector<Product> products;
    std::vector<Customer> customers;
    std::vector<Order> orders;
    std::map<int, std::vector<CartItem>> carts;   // one cart per customer ID (memory only)
    FileManager fileManager;

    int nextCustomerId;
    int nextOrderId;
    int loggedInCustomerId;     // 0 means "nobody is logged in"
    bool inputClosed;           // set when the input stream ends (EOF)
    bool adminLoggedIn;

    // ---- Input helpers ----
    bool readLine(const std::string& prompt, std::string& out);
    bool readChoice(int& choice);
    bool promptValidated(const std::string& prompt,
                         const std::function<std::string(const std::string&)>& validator,
                         std::string& value);

    // ---- Lookup helpers ----
    const Product* findProductById(int id) const;
    Product* findProductById(int id);
    const Customer* findCustomerByEmail(const std::string& email) const;
    const Customer* findCustomerById(int id) const;
    bool emailExists(const std::string& email) const;
    bool requireLogin() const;

    // ---- Menus and screens ----
    void displayMainMenu() const;
    void displayCustomerMenu() const;
    void handleChoice(int choice, bool& running);
    void showAbout() const;
    void viewProductsScreen();
    void searchScreen();
    void promptProductDetails();
    void showProductDetails(const Product& product) const;
    void registrationScreen();
    void loginScreen();
    void customerPanel();
    void showAccount() const;
    void adminLoginScreen();
    void adminPanel();
    void saveAllData();
    void adminProducts(int action = 0);
    void adminCustomers() const;
    void adminOrders();
    void adminStatistics() const;

    // ---- Cart and order screens ----
    bool readNumber(const std::string& prompt, int& value, bool& valid);
    void showCart() const;
    void cartScreen();
    void cartAddScreen();
    void cartUpdateScreen();
    void cartRemoveScreen();
    bool checkoutScreen();
    void viewOrdersScreen();
    void showOrderDetails(const Order& order) const;

public:
    Store();

    // ---- Product management ----
    bool addProduct(const Product& product, std::string& error);
    void loadSampleProducts();
    void showAllProducts() const;
    int searchByName(const std::string& term) const;       // returns number of matches
    int searchByCategory(const std::string& term) const;   // returns number of matches

    // ---- Customer management ----
    bool registerCustomer(const std::string& name, const std::string& email,
                          const std::string& password, const std::string& phone,
                          std::string& message);
    bool login(const std::string& email, const std::string& password);
    void logout();
    bool isLoggedIn() const { return loggedInCustomerId != 0; }
    int getLoggedInCustomerId() const { return loggedInCustomerId; }

    // ---- Shopping cart (all of these work on the cart of the given customer ID) ----
    // Each returns true on success; "message" holds the reason when it returns false.
    bool addToCart(int customerId, int productId, int quantity, std::string& message);
    bool updateCartQuantity(int customerId, int productId, int newQuantity, std::string& message);
    bool removeFromCart(int customerId, int productId, std::string& message);
    std::vector<CartItem> getCart(int customerId) const;            // a copy of the cart
    long long getCartTotalPaise(int customerId) const;              // total in paise
    std::vector<std::string> getCartProblems(int customerId) const; // current stock problems

    // ---- Orders ----
    // Checks the cart again, then creates the order, reduces stock and clears the cart.
    // Either everything happens or nothing does. On success orderId is the new Order ID.
    bool placeOrder(int customerId, std::string& message, int& orderId);
    std::vector<Order> getCustomerOrders(int customerId) const;     // only this customer's orders
    int getProductStock(int productId) const;                       // -1 if no such product

    // ---- Application loop ----
    void run();
};

#endif // STORE_H
