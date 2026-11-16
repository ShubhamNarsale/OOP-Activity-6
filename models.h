// models.h - Data model classes for TechStore (Product, Customer, OrderItem, Order)
#ifndef MODELS_H
#define MODELS_H

#include <cmath>
#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>
#include "password_utils.h"

// ---------------------------------------------------------------
// Limits and money helpers (shared by Product, OrderItem and Order)
// Money totals are calculated in whole paise (long long), never with double,
// so cart and order totals cannot pick up rounding errors.
// ---------------------------------------------------------------
constexpr double MAX_PRICE = 10000000.0;   // Rs. 1,00,00,000 per unit
constexpr int MAX_STOCK = 1000000;         // also the largest quantity on one line

// Rupees (double) -> paise (whole number). 19.99 -> 1999
inline long long toPaise(double rupees) {
    return static_cast<long long>(std::llround(rupees * 100.0));
}

// "YYYY-MM-DD HH:MM:SS" in the computer's local time
inline std::string currentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    char buffer[32];
    if (local == nullptr || std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) == 0) {
        return "unknown";
    }
    return buffer;
}

// ---------------------------------------------------------------
// Product: one item sold in the store
// ---------------------------------------------------------------
class Product {
private:
    int id;
    std::string name;
    std::string category;
    double price;
    int stock;
    std::string description;

    // True if the text has at least one character that is not whitespace
    static bool hasText(const std::string& text) {
        return text.find_first_not_of(" \t\r\n") != std::string::npos;
    }

public:
    // Returns "" if the values are acceptable, otherwise a message saying what is wrong.
    static std::string validate(int id, const std::string& name, const std::string& category,
                                double price, int stock) {
        if (id <= 0) return "Product ID must be a positive number.";
        if (!hasText(name)) return "Product name cannot be empty.";
        if (!hasText(category)) return "Product category cannot be empty.";
        if (!std::isfinite(price) || price <= 0.0) return "Price must be greater than zero.";
        if (price > MAX_PRICE) return "Price cannot exceed Rs. 1,00,00,000.";
        if (stock < 0) return "Stock cannot be negative.";
        if (stock > MAX_STOCK) return "Stock cannot exceed 1,000,000.";
        return "";
    }

    // Empty placeholder object; it is not a valid product and is not used by the store.
    Product() : id(0), price(0.0), stock(0) {}

    // Throws std::invalid_argument if any value is invalid, so an invalid Product
    // can never be created through this constructor.
    Product(int id, const std::string& name, const std::string& category,
            double price, int stock, const std::string& description)
        : id(id), name(name), category(category),
          price(price), stock(stock), description(description) {
        std::string problem = validate(id, name, category, price, stock);
        if (!problem.empty()) throw std::invalid_argument(problem);
    }

    // Getters
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::string& getCategory() const { return category; }
    double getPrice() const { return price; }
    long long getPricePaise() const { return toPaise(price); }
    int getStock() const { return stock; }
    const std::string& getDescription() const { return description; }

    // Setters (name, category, price and stock setters reject invalid values and return false)
    bool setName(const std::string& newName) {
        if (!hasText(newName)) return false;
        name = newName;
        return true;
    }
    bool setCategory(const std::string& newCategory) {
        if (!hasText(newCategory)) return false;
        category = newCategory;
        return true;
    }
    bool setPrice(double newPrice) {
        if (!std::isfinite(newPrice) || newPrice <= 0.0 || newPrice > MAX_PRICE) return false;
        price = newPrice;
        return true;
    }
    bool setStock(int newStock) {
        if (newStock < 0 || newStock > MAX_STOCK) return false;
        stock = newStock;
        return true;
    }
    void setDescription(const std::string& newDesc) { description = newDesc; }

    // Helper used later by order handling
    bool isInStock(int quantity) const { return quantity > 0 && stock >= quantity; }
};

// ---------------------------------------------------------------
// Customer: a registered user of the store
// The password is never kept as plain text - only a salted hash.
// ---------------------------------------------------------------
class Customer {
private:
    int id;
    std::string name;
    std::string email;
    std::string passwordHash;   // record created by PasswordUtils::hashPassword()
    std::string phone;

public:
    Customer() : id(0) {}

    Customer(int id, const std::string& name, const std::string& email,
             const std::string& passwordHash, const std::string& phone)
        : id(id), name(name), email(email), passwordHash(passwordHash), phone(phone) {}

    // Getters (there is deliberately no getter for the password or its hash)
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::string& getEmail() const { return email; }
    const std::string& getPhone() const { return phone; }
    const std::string& getPasswordRecord() const { return passwordHash; }

    // Setters
    void setName(const std::string& newName) { name = newName; }
    void setEmail(const std::string& newEmail) { email = newEmail; }
    void setPhone(const std::string& newPhone) { phone = newPhone; }
    void setPassword(const std::string& plainPassword) {
        passwordHash = PasswordUtils::hashPassword(plainPassword);
    }

    bool checkPassword(const std::string& attempt) const {
        return PasswordUtils::verifyPassword(attempt, passwordHash);
    }
};

// ---------------------------------------------------------------
// CartItem: one line in a customer's shopping cart.
// A separate small class is needed because a cart line must NOT freeze the price:
// it keeps only the product ID and quantity, and the name and price are looked up
// from the product at display/checkout time. (OrderItem, below, is the opposite:
// it keeps a copy of the name and price from the moment of purchase.)
// ---------------------------------------------------------------
class CartItem {
private:
    int productId;
    int quantity;

public:
    CartItem(int productId, int quantity) : productId(productId), quantity(quantity) {}

    int getProductId() const { return productId; }
    int getQuantity() const { return quantity; }
    void setQuantity(int newQuantity) { quantity = newQuantity; }
};

// ---------------------------------------------------------------
// OrderItem: one product line inside an order.
// It stores the product ID, name, quantity and unit price as they were when the
// order was placed, so later price or name changes never alter an old order.
// ---------------------------------------------------------------
class OrderItem {
private:
    int productId;
    std::string productName;
    int quantity;
    long long unitPricePaise;

    static bool hasText(const std::string& text) {
        return text.find_first_not_of(" \t\r\n") != std::string::npos;
    }

public:
    // Returns "" if the values are acceptable, otherwise the reason they are not.
    static std::string validate(int productId, const std::string& productName,
                                int quantity, double unitPrice) {
        if (productId <= 0) return "Order item needs a valid product ID.";
        if (!hasText(productName)) return "Order item needs a product name.";
        if (quantity <= 0) return "Order item quantity must be greater than zero.";
        if (quantity > MAX_STOCK) return "Order item quantity is too large.";
        if (!std::isfinite(unitPrice) || unitPrice <= 0.0) return "Order item price must be greater than zero.";
        if (unitPrice > MAX_PRICE) return "Order item price is too large.";
        return "";
    }

    // Empty placeholder; it is not valid and Order::addItem() rejects it.
    OrderItem() : productId(0), quantity(0), unitPricePaise(0) {}

    // Throws std::invalid_argument if any value is invalid.
    OrderItem(int productId, const std::string& productName, int quantity, double unitPrice)
        : productId(productId), productName(productName),
          quantity(quantity), unitPricePaise(0) {
        std::string problem = validate(productId, productName, quantity, unitPrice);
        if (!problem.empty()) throw std::invalid_argument(problem);
        unitPricePaise = toPaise(unitPrice);
        if (unitPricePaise <= 0) throw std::invalid_argument("Order item price must be at least Rs. 0.01.");
    }

    bool isValid() const {
        return productId > 0 && hasText(productName) && quantity > 0 && quantity <= MAX_STOCK &&
               unitPricePaise > 0 && unitPricePaise <= toPaise(MAX_PRICE);
    }

    int getProductId() const { return productId; }
    const std::string& getProductName() const { return productName; }
    int getQuantity() const { return quantity; }
    long long getUnitPricePaise() const { return unitPricePaise; }
    long long getSubtotalPaise() const { return unitPricePaise * quantity; }
    double getUnitPrice() const { return static_cast<double>(unitPricePaise) / 100.0; }
    double getSubtotal() const { return static_cast<double>(getSubtotalPaise()) / 100.0; }
};

// ---------------------------------------------------------------
// Order: one customer's order, which can hold several products.
// The date is the local date and time when the Order object was created
// (that is, when the customer confirmed the order).
// ---------------------------------------------------------------
class Order {
private:
    int orderId;
    int customerId;
    std::vector<OrderItem> items;
    std::string status;
    std::string orderDate;

public:
    Order() : orderId(0), customerId(0), status("Pending"), orderDate(currentTimestamp()) {}

    Order(int orderId, int customerId)
        : orderId(orderId), customerId(customerId), status("Pending"),
          orderDate(currentTimestamp()) {}

    // Adds a product line; rejects an invalid item (bad quantity, price, name or ID)
    bool addItem(const OrderItem& item) {
        if (!item.isValid()) return false;
        items.push_back(item);
        return true;
    }

    // Getters
    int getOrderId() const { return orderId; }
    int getCustomerId() const { return customerId; }
    const std::vector<OrderItem>& getItems() const { return items; }
    const std::string& getStatus() const { return status; }
    const std::string& getOrderDate() const { return orderDate; }

    // The total is calculated from the items, so it can never be out of date
    long long getTotalPaise() const {
        long long total = 0;
        for (const OrderItem& item : items) total += item.getSubtotalPaise();
        return total;
    }
    double getTotalAmount() const { return static_cast<double>(getTotalPaise()) / 100.0; }

    // Setter (only the status changes after an order is placed; current release will use it)
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void restoreDate(const std::string& value) { orderDate = value; }
};

#endif // MODELS_H
