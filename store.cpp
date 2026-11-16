// store.cpp - Menus, product management and customer management
#include "store.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <map>
#include <iostream>
#include <stdexcept>
#include <string>
#include <fstream>

namespace {

const std::string LINE(40, '=');
const std::string THIN(40, '-');
const int MAX_ATTEMPTS = 3;

// ---------- Small text helpers ----------

std::string trim(const std::string& text) {
    const std::string spaces = " \t\r\n";
    std::size_t first = text.find_first_not_of(spaces);
    if (first == std::string::npos) return "";
    std::size_t last = text.find_last_not_of(spaces);
    return text.substr(first, last - first + 1);
}

std::string toLower(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

// Accepts digits only (up to 9 of them). Letters, signs, spaces and empty text fail.
bool parseNumber(const std::string& text, int& value) {
    if (text.empty() || text.size() > 9 ||
        text.find_first_not_of("0123456789") != std::string::npos) {
        return false;
    }
    value = std::stoi(text);
    return true;
}

// Rs. 124999.5 -> "Rs. 1,24,999.50" (Indian digit grouping)
std::string formatPaise(long long paise) {
    std::string rupees = std::to_string(paise / 100);
    int remainder = static_cast<int>(paise % 100);

    std::string grouped = rupees;
    if (rupees.size() > 3) {
        std::string lastThree = rupees.substr(rupees.size() - 3);
        std::string rest = rupees.substr(0, rupees.size() - 3);
        std::string restGrouped;
        int count = 0;
        for (int i = static_cast<int>(rest.size()) - 1; i >= 0; --i) {
            restGrouped.insert(restGrouped.begin(), rest[static_cast<std::size_t>(i)]);
            if (++count % 2 == 0 && i > 0) restGrouped.insert(restGrouped.begin(), ',');
        }
        grouped = restGrouped + "," + lastThree;
    }
    char decimals[8];
    std::snprintf(decimals, sizeof(decimals), ".%02d", remainder);
    return "Rs. " + grouped + decimals;
}

std::string formatPrice(double price) {
    return formatPaise(toPaise(price));
}

std::string shorten(const std::string& text, std::size_t width) {
    if (text.size() <= width) return text;
    return text.substr(0, width - 3) + "...";
}

// ---------- Registration validators (return "" when the value is fine) ----------

std::string validateName(const std::string& name) {
    if (name.empty()) return "Name cannot be empty.";
    if (name.size() > 50) return "Name is too long (maximum 50 characters).";
    bool hasLetter = false;
    for (char c : name) {
        if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
    }
    if (!hasLetter) return "Name must contain at least one letter.";
    return "";
}

std::string validateEmail(const std::string& email) {
    if (email.empty()) return "Email cannot be empty.";
    if (email.size() > 60) return "Email is too long (maximum 60 characters).";
    if (email.find_first_of(" \t") != std::string::npos) return "Email cannot contain spaces.";

    std::size_t at = email.find('@');
    if (at == std::string::npos || at != email.rfind('@')) {
        return "Email must contain exactly one '@' (example: name@example.com).";
    }
    std::string local = email.substr(0, at);
    std::string domain = email.substr(at + 1);
    if (local.empty()) return "Email must have text before the '@'.";

    std::size_t dot = domain.rfind('.');
    if (dot == std::string::npos || dot == 0 || domain[0] == '.' ||
        domain.find("..") != std::string::npos || domain.size() - dot - 1 < 2) {
        return "Email domain must look like example.com.";
    }
    return "";
}

std::string validatePassword(const std::string& password) {
    if (password.size() < 8) return "Password must be at least 8 characters long.";
    if (password.size() > 32) return "Password is too long (maximum 32 characters).";
    bool hasLetter = false, hasDigit = false;
    for (char c : password) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isspace(u)) return "Password cannot contain spaces.";
        if (std::isalpha(u)) hasLetter = true;
        if (std::isdigit(u)) hasDigit = true;
    }
    if (!hasLetter || !hasDigit) return "Password must contain at least one letter and one digit.";
    return "";
}

std::string validatePhone(const std::string& phone) {
    if (phone.empty()) return "Phone number cannot be empty.";
    if (phone.find_first_not_of("0123456789") != std::string::npos) {
        return "Phone number must contain digits only (no spaces, '+' or '-').";
    }
    if (phone.size() != 10) return "Phone number must be exactly 10 digits.";
    return "";
}

// ---------- Product table ----------

void printProductTable(const std::vector<const Product*>& list) {
    std::cout << "\n" << std::left
              << std::setw(5) << "ID"
              << std::setw(37) << "Name"
              << std::setw(13) << "Category"
              << std::setw(16) << "Price"
              << "Stock\n"
              << std::string(76, '-') << "\n";
    for (const Product* p : list) {
        std::cout << std::left
                  << std::setw(5) << p->getId()
                  << std::setw(37) << shorten(p->getName(), 35)
                  << std::setw(13) << shorten(p->getCategory(), 11)
                  << std::setw(16) << formatPrice(p->getPrice())
                  << p->getStock() << "\n";
    }
    std::cout << std::string(76, '-') << "\n";
}

// Case-insensitive search. useCategory = true searches the category instead of the name.
int searchProducts(const std::vector<Product>& products, const std::string& rawTerm,
                   bool useCategory, const std::string& label) {
    std::string term = trim(rawTerm);
    if (term.empty()) {
        std::cout << "\n[ERROR] Search term cannot be empty.\n";
        return 0;
    }
    if (products.empty()) {
        std::cout << "\n[INFO] No products are available at the moment.\n";
        return 0;
    }

    std::string needle = toLower(term);
    std::vector<const Product*> matches;
    for (const Product& p : products) {
        const std::string& field = useCategory ? p.getCategory() : p.getName();
        if (toLower(field).find(needle) != std::string::npos) matches.push_back(&p);
    }

    if (matches.empty()) {
        std::cout << "\n[INFO] No products found for " << label << " \"" << term << "\".\n";
        return 0;
    }
    std::cout << "\n" << matches.size() << " product(s) found for " << label
              << " \"" << term << "\":";
    printProductTable(matches);
    return static_cast<int>(matches.size());
}

} // namespace

// =====================================================================
// Construction and input helpers
// =====================================================================

Store::Store() : nextCustomerId(1001), nextOrderId(5001), loggedInCustomerId(0), inputClosed(false), adminLoggedIn(false) {
    bool productsPresent = false;
    { std::ifstream f(fileManager.getProductFile()); productsPresent = f.good(); }
    bool okP = fileManager.loadProducts(products);
    bool okC = fileManager.loadCustomers(customers);
    bool okO = fileManager.loadOrders(orders);
    if (!okP) std::cout << "[WARNING] Product data could not be loaded. Existing file was left untouched.\n";
    if (!okC) std::cout << "[WARNING] Customer data could not be loaded. Existing file was left untouched.\n";
    if (!okO) std::cout << "[WARNING] Order data could not be loaded. Existing file was left untouched.\n";
    for (const Customer& c : customers) if (c.getId() >= nextCustomerId) nextCustomerId = c.getId() + 1;
    for (const Order& o : orders) if (o.getOrderId() >= nextOrderId) nextOrderId = o.getOrderId() + 1;
    if (!productsPresent && okP) { loadSampleProducts(); fileManager.saveProducts(products); }
}

// Reads one line (trimmed). Returns false if the input stream has ended.
bool Store::readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        inputClosed = true;
        return false;
    }
    out = trim(line);
    return true;
}

// Reads a menu choice as text, so letters never cause an infinite loop.
// choice is set to -1 when the input is not a valid number.
bool Store::readChoice(int& choice) {
    std::string line;
    if (!std::getline(std::cin, line)) {
        inputClosed = true;
        return false;
    }
    line = trim(line);
    if (line.size() > 3 || !parseNumber(line, choice)) choice = -1;
    return true;
}

// Asks for a value until the validator accepts it (MAX_ATTEMPTS tries).
bool Store::promptValidated(const std::string& prompt,
                            const std::function<std::string(const std::string&)>& validator,
                            std::string& value) {
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        if (!readLine(prompt, value)) return false;
        std::string error = validator(value);
        if (error.empty()) return true;
        std::cout << "[ERROR] " << error << "\n";
        if (attempt < MAX_ATTEMPTS) {
            std::cout << "        Please try again (" << (MAX_ATTEMPTS - attempt)
                      << " attempt(s) left).\n";
        }
    }
    std::cout << "\n[INFO] Too many invalid entries. Registration cancelled.\n";
    return false;
}

// =====================================================================
// Lookup helpers
// =====================================================================

const Product* Store::findProductById(int id) const {
    for (const Product& p : products) {
        if (p.getId() == id) return &p;
    }
    return nullptr;
}

Product* Store::findProductById(int id) {
    for (Product& p : products) {
        if (p.getId() == id) return &p;
    }
    return nullptr;
}

const Customer* Store::findCustomerByEmail(const std::string& email) const {
    std::string wanted = toLower(trim(email));
    for (const Customer& c : customers) {
        if (c.getEmail() == wanted) return &c;
    }
    return nullptr;
}

const Customer* Store::findCustomerById(int id) const {
    for (const Customer& c : customers) {
        if (c.getId() == id) return &c;
    }
    return nullptr;
}

bool Store::emailExists(const std::string& email) const {
    return findCustomerByEmail(email) != nullptr;
}

// Guard for customer-only operations
bool Store::requireLogin() const {
    if (!isLoggedIn()) {
        std::cout << "\n[ERROR] Please login as a customer to access this feature.\n";
        return false;
    }
    return true;
}

// =====================================================================
// Product management
// =====================================================================

bool Store::addProduct(const Product& product, std::string& error) {
    if (product.getId() <= 0) error = "Product ID must be a positive number.";
    else if (findProductById(product.getId()) != nullptr) error = "Product ID already exists.";
    else if (trim(product.getName()).empty()) error = "Product name cannot be empty.";
    else if (trim(product.getCategory()).empty()) error = "Product category cannot be empty.";
    else if (!(product.getPrice() > 0.0) || !std::isfinite(product.getPrice())) error = "Price must be greater than zero.";
    else if (product.getStock() < 0) error = "Stock cannot be negative.";
    else {
        products.push_back(product);
        error.clear();
        return true;
    }
    return false;
}

void Store::loadSampleProducts() {
    if (!products.empty()) return;
    try {
        // Sample data for testing; prices are example values, not live prices.
        const Product samples[] = {
            Product(1,  "Dell Inspiron 15 3520",            "Laptops",    52990.0, 12, "15.6-inch FHD laptop, Intel Core i5, 16 GB RAM, 512 GB SSD"),
            Product(2,  "HP Pavilion 14",                   "Laptops",    67990.0,  8, "14-inch FHD laptop, Intel Core i5, 16 GB RAM, 512 GB SSD"),
            Product(3,  "Samsung Galaxy S23 FE",            "Mobiles",    39999.0, 20, "6.4-inch AMOLED display, 8 GB RAM, 128 GB storage"),
            Product(4,  "OnePlus Nord CE4",                 "Mobiles",    24999.0, 25, "6.7-inch AMOLED display, 8 GB RAM, 128 GB storage"),
            Product(5,  "Sony WH-CH520 Headphones",         "Headphones",  4990.0, 30, "Wireless on-ear headphones, up to 50 hours of battery"),
            Product(6,  "boAt Rockerz 450",                 "Headphones",  1499.0, 50, "Bluetooth on-ear headphones, up to 15 hours of battery"),
            Product(7,  "Logitech K380 Keyboard",           "Keyboards",   2995.0, 18, "Compact Bluetooth keyboard, works with up to 3 devices"),
            Product(8,  "Redragon Kumara K552",             "Keyboards",   3299.0, 15, "Wired mechanical keyboard with RGB backlight"),
            Product(9,  "Logitech M331 Silent Plus",        "Mice",        1295.0, 40, "Wireless mouse with silent clicks, 24-month battery life"),
            Product(10, "Zebronics Zeb-Transformer",        "Mice",         599.0, 60, "Wired gaming mouse with RGB lighting"),
            Product(11, "Dell SE2425H 24-inch Monitor",     "Monitors",    8499.0, 10, "23.8-inch Full HD VA monitor, 75 Hz"),
            Product(12, "LG 27MP400 27-inch Monitor",       "Monitors",   11999.0,  7, "27-inch Full HD IPS monitor, 75 Hz, AMD FreeSync")};

        for (const Product& sample : samples) {
            std::string error;
            if (!addProduct(sample, error)) {
                std::cout << "[WARNING] Sample product skipped: " << error << "\n";
            }
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "[WARNING] Sample products not loaded: " << e.what() << "\n";
    }
}

void Store::showAllProducts() const {
    std::cout << "\n" << LINE << "\n"
              << "            ALL PRODUCTS\n"
              << LINE << "\n";
    if (products.empty()) {
        std::cout << "\n[INFO] No products are available at the moment.\n";
        return;
    }
    std::vector<const Product*> all;
    for (const Product& p : products) all.push_back(&p);
    std::cout << "\n" << all.size() << " product(s) available.";
    printProductTable(all);
}

int Store::searchByName(const std::string& term) const {
    return searchProducts(products, term, false, "name");
}

int Store::searchByCategory(const std::string& term) const {
    return searchProducts(products, term, true, "category");
}

void Store::showProductDetails(const Product& product) const {
    std::cout << "\n" << LINE << "\n"
              << "           PRODUCT DETAILS\n"
              << LINE << "\n"
              << "Product ID  : " << product.getId() << "\n"
              << "Name        : " << product.getName() << "\n"
              << "Category    : " << product.getCategory() << "\n"
              << "Price       : " << formatPrice(product.getPrice()) << "\n"
              << "Available   : ";
    if (product.getStock() > 0) std::cout << product.getStock() << " unit(s)\n";
    else std::cout << "Out of stock\n";
    std::cout << "Description : " << product.getDescription() << "\n"
              << THIN << "\n";
}

// Lets the user look at the full details of a product by its ID.
void Store::promptProductDetails() {
    while (true) {
        std::string line;
        if (!readLine("\nEnter Product ID for details (0 to go back): ", line)) return;
        int id = 0;
        if (!parseNumber(line, id)) {
            std::cout << "[ERROR] Please enter a valid Product ID (a number).\n";
            continue;
        }
        if (id == 0) return;
        const Product* product = findProductById(id);
        if (product == nullptr) {
            std::cout << "[ERROR] No product found with ID " << id << ".\n";
            continue;
        }
        showProductDetails(*product);
    }
}

void Store::viewProductsScreen() {
    showAllProducts();
    if (!products.empty()) promptProductDetails();
}

void Store::searchScreen() {
    while (!inputClosed) {
        std::cout << "\n" << LINE << "\n"
                  << "           SEARCH PRODUCTS\n"
                  << LINE << "\n\n"
                  << "1. Search by Name\n"
                  << "2. Search by Category\n"
                  << "0. Back\n\n"
                  << "Enter your choice: ";
        int choice = -1;
        if (!readChoice(choice)) return;

        if (choice == 0) return;
        if (choice != 1 && choice != 2) {
            std::cout << "\n[ERROR] Invalid choice. Please enter 1, 2 or 0.\n";
            continue;
        }

        std::string term;
        const char* prompt = (choice == 1) ? "Enter product name to search: "
                                           : "Enter category to search: ";
        if (!readLine(prompt, term)) return;

        int found = (choice == 1) ? searchByName(term) : searchByCategory(term);
        if (found > 0) promptProductDetails();
    }
}

// =====================================================================
// Customer management
// =====================================================================

bool Store::registerCustomer(const std::string& rawName, const std::string& rawEmail,
                             const std::string& password, const std::string& rawPhone,
                             std::string& message) {
    std::string name = trim(rawName);
    std::string email = toLower(trim(rawEmail));
    std::string phone = trim(rawPhone);

    std::string error = validateName(name);
    if (error.empty()) error = validateEmail(email);
    if (error.empty() && emailExists(email)) {
        error = "This email address is already registered. Use a different email or log in.";
    }
    if (error.empty()) error = validatePassword(password);
    if (error.empty()) error = validatePhone(phone);
    if (!error.empty()) {
        message = error;
        return false;
    }

    std::string record;
    try {
        record = PasswordUtils::hashPassword(password);
    } catch (const std::exception&) {
        message = "Registration failed: the system's secure random generator is unavailable.";
        return false;
    }
    customers.emplace_back(nextCustomerId, name, email, record, phone);
    message = "Registration successful! Your Customer ID is " + std::to_string(nextCustomerId) + ".";
    ++nextCustomerId;
    fileManager.saveCustomers(customers);
    return true;
}

bool Store::login(const std::string& email, const std::string& password) {
    const Customer* customer = findCustomerByEmail(email);
    if (customer != nullptr && customer->checkPassword(password)) {
        loggedInCustomerId = customer->getId();
        return true;
    }
    return false;
}

void Store::logout() {
    loggedInCustomerId = 0;
}

void Store::registrationScreen() {
    std::cout << "\n" << LINE << "\n"
              << "        CUSTOMER REGISTRATION\n"
              << LINE << "\n"
              << "Password rules: 8-32 characters, at least one letter and one digit, no spaces.\n"
              << "Phone number : exactly 10 digits.\n\n";

    std::string name, email, password, confirm, phone;

    if (!promptValidated("Full Name        : ", validateName, name)) return;

    auto emailValidator = [this](const std::string& value) -> std::string {
        std::string lowered = toLower(value);
        std::string error = validateEmail(lowered);
        if (error.empty() && emailExists(lowered)) {
            error = "This email address is already registered. Use a different email or log in.";
        }
        return error;
    };
    if (!promptValidated("Email Address    : ", emailValidator, email)) return;

    if (!promptValidated("Password         : ", validatePassword, password)) return;

    auto confirmValidator = [&password](const std::string& value) -> std::string {
        return (value == password) ? "" : "Passwords do not match.";
    };
    if (!promptValidated("Confirm Password : ", confirmValidator, confirm)) return;

    if (!promptValidated("Phone Number     : ", validatePhone, phone)) return;

    std::string message;
    if (registerCustomer(name, email, password, phone, message)) {
        std::cout << "\n[SUCCESS] " << message << "\n"
                  << "          You can now log in from the main menu.\n"
                  << "          Your account is saved permanently. Shopping carts are session-based.\n";
    } else {
        std::cout << "\n[ERROR] " << message << "\n";
    }
}

void Store::loginScreen() {
    if (isLoggedIn()) {
        std::cout << "\n[INFO] You are already logged in.\n";
        customerPanel();
        return;
    }

    std::cout << "\n" << LINE << "\n"
              << "            CUSTOMER LOGIN\n"
              << LINE << "\n\n";

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        std::string email, password;
        if (!readLine("Email    : ", email)) return;
        if (!readLine("Password : ", password)) return;

        if (login(email, password)) {
            const Customer* customer = findCustomerById(loggedInCustomerId);
            std::cout << "\n[SUCCESS] Login successful. Welcome, " << customer->getName() << "!\n";
            customerPanel();
            return;
        }

        std::cout << "\n[ERROR] Invalid email or password.\n";
        if (attempt < MAX_ATTEMPTS) {
            std::cout << "        Please try again (" << (MAX_ATTEMPTS - attempt)
                      << " attempt(s) left).\n\n";
        }
    }
    std::cout << "\n[INFO] Too many failed attempts. Returning to the main menu.\n";
}

void Store::displayCustomerMenu() const {
    std::cout << "\n" << LINE << "\n"
              << "             CUSTOMER PANEL\n"
              << LINE << "\n\n"
              << "1. View Products\n"
              << "2. Search Products\n"
              << "3. View My Account\n"
              << "4. Shopping Cart\n"
              << "5. Place Order\n"
              << "6. View My Orders\n"
              << "0. Logout\n\n"
              << "Enter your choice: ";
}

void Store::customerPanel() {
    while (isLoggedIn() && !inputClosed) {
        displayCustomerMenu();
        int choice = -1;
        if (!readChoice(choice)) return;

        switch (choice) {
            case 1: viewProductsScreen(); break;
            case 2: searchScreen(); break;
            case 3: showAccount(); break;
            case 4: cartScreen(); break;
            case 5: checkoutScreen(); break;
            case 6: viewOrdersScreen(); break;
            case 0:
                logout();
                std::cout << "\n[SUCCESS] You have been logged out.\n";
                break;
            default:
                std::cout << "\n[ERROR] Invalid choice. Please enter a number from 0 to 6.\n";
                break;
        }
    }
}

// Shows account details - never the password or its hash.
void Store::showAccount() const {
    if (!requireLogin()) return;
    const Customer* customer = findCustomerById(loggedInCustomerId);
    if (customer == nullptr) {
        std::cout << "\n[ERROR] Account not found.\n";
        return;
    }
    std::cout << "\n" << LINE << "\n"
              << "              MY ACCOUNT\n"
              << LINE << "\n"
              << "Customer ID : " << customer->getId() << "\n"
              << "Name        : " << customer->getName() << "\n"
              << "Email       : " << customer->getEmail() << "\n"
              << "Phone       : " << customer->getPhone() << "\n"
              << THIN << "\n";
}

// =====================================================================
// Shopping cart business logic
// Carts live in memory only, in a map keyed by customer ID. Every function below
// receives the customer ID, so one customer's cart is never mixed with another's.
// =====================================================================

bool Store::addToCart(int customerId, int productId, int quantity, std::string& message) {
    if (findCustomerById(customerId) == nullptr) { message = "Customer not found."; return false; }
    if (quantity <= 0) { message = "Quantity must be greater than zero."; return false; }

    const Product* product = findProductById(productId);
    if (product == nullptr) { message = "No product found with ID " + std::to_string(productId) + "."; return false; }
    if (product->getStock() <= 0) { message = product->getName() + " is out of stock."; return false; }

    std::vector<CartItem>& cart = carts[customerId];
    CartItem* existing = nullptr;
    for (CartItem& item : cart) {
        if (item.getProductId() == productId) existing = &item;
    }
    int inCart = (existing != nullptr) ? existing->getQuantity() : 0;

    // Compare without adding first, so a huge quantity cannot overflow
    if (quantity > product->getStock() - inCart) {
        message = "Only " + std::to_string(product->getStock()) + " unit(s) of " + product->getName() +
                  " in stock";
        if (inCart > 0) message += " (you already have " + std::to_string(inCart) + " in your cart)";
        message += ".";
        return false;
    }

    if (existing != nullptr) {
        existing->setQuantity(inCart + quantity);    // same product again: no duplicate line
        message = "Cart updated: " + product->getName() + " now has quantity " +
                  std::to_string(inCart + quantity) + ".";
    } else {
        cart.emplace_back(productId, quantity);
        message = product->getName() + " added to your cart.";
    }
    return true;
}

bool Store::updateCartQuantity(int customerId, int productId, int newQuantity, std::string& message) {
    auto found = carts.find(customerId);
    CartItem* line = nullptr;
    if (found != carts.end()) {
        for (CartItem& item : found->second) {
            if (item.getProductId() == productId) line = &item;
        }
    }
    if (line == nullptr) { message = "Product " + std::to_string(productId) + " is not in your cart."; return false; }
    if (newQuantity <= 0) {
        message = "Quantity must be greater than zero. Use 'Remove Product' to delete an item.";
        return false;
    }
    const Product* product = findProductById(productId);
    if (product == nullptr) { message = "This product is no longer available."; return false; }
    if (newQuantity > product->getStock()) {
        message = "Only " + std::to_string(product->getStock()) + " unit(s) of " + product->getName() + " in stock.";
        return false;
    }
    line->setQuantity(newQuantity);
    message = "Quantity of " + product->getName() + " changed to " + std::to_string(newQuantity) + ".";
    return true;
}

bool Store::removeFromCart(int customerId, int productId, std::string& message) {
    auto found = carts.find(customerId);
    if (found != carts.end()) {
        std::vector<CartItem>& cart = found->second;
        for (std::size_t i = 0; i < cart.size(); ++i) {
            if (cart[i].getProductId() == productId) {
                const Product* product = findProductById(productId);
                message = (product != nullptr ? product->getName() : "Product") + " removed from your cart.";
                cart.erase(cart.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
        }
    }
    message = "Product " + std::to_string(productId) + " is not in your cart.";
    return false;
}

std::vector<CartItem> Store::getCart(int customerId) const {
    auto found = carts.find(customerId);
    if (found == carts.end()) return std::vector<CartItem>();
    return found->second;
}

long long Store::getCartTotalPaise(int customerId) const {
    long long total = 0;
    for (const CartItem& item : getCart(customerId)) {
        const Product* product = findProductById(item.getProductId());
        if (product != nullptr) total += product->getPricePaise() * item.getQuantity();
    }
    return total;
}

// Lists every reason the cart cannot be checked out right now (empty list = OK).
std::vector<std::string> Store::getCartProblems(int customerId) const {
    std::vector<std::string> problems;
    std::vector<CartItem> cart = getCart(customerId);
    if (cart.empty()) {
        problems.push_back("Your cart is empty.");
        return problems;
    }
    for (const CartItem& item : cart) {
        const Product* product = findProductById(item.getProductId());
        if (product == nullptr) {
            problems.push_back("Product " + std::to_string(item.getProductId()) + " is no longer available.");
        } else if (item.getQuantity() <= 0) {
            problems.push_back(product->getName() + ": quantity must be greater than zero.");
        } else if (product->getStock() <= 0) {
            problems.push_back(product->getName() + " is now out of stock.");
        } else if (item.getQuantity() > product->getStock()) {
            problems.push_back(product->getName() + ": you want " + std::to_string(item.getQuantity()) +
                               " but only " + std::to_string(product->getStock()) + " in stock.");
        }
    }
    return problems;
}

// =====================================================================
// Order business logic
// =====================================================================

bool Store::placeOrder(int customerId, std::string& message, int& orderId) {
    orderId = 0;
    if (findCustomerById(customerId) == nullptr) { message = "Customer not found."; return false; }

    // Validate the complete cart before changing any store data.
    std::vector<std::string> problems = getCartProblems(customerId);
    if (!problems.empty()) { message = problems.front(); return false; }

    // Build the order in a temporary object so invalid items cannot create a partial order.
    Order order(nextOrderId, customerId);
    std::vector<CartItem> cart = getCart(customerId);
    for (const CartItem& line : cart) {
        const Product* product = findProductById(line.getProductId());
        try {
            OrderItem item(product->getId(), product->getName(), line.getQuantity(), product->getPrice());
            if (!order.addItem(item)) { message = "Could not add " + product->getName() + " to the order."; return false; }
        } catch (const std::invalid_argument& e) {
            message = e.what();
            return false;
        }
    }

    // Once validation succeeds, update stock, save the order and clear the cart.
    for (const CartItem& line : cart) {
        Product* product = findProductById(line.getProductId());
        product->setStock(product->getStock() - line.getQuantity());
    }
    orders.push_back(order);
    orderId = nextOrderId;
    ++nextOrderId;
    carts.erase(customerId);
    fileManager.saveProducts(products); fileManager.saveOrders(orders);
    message = "Order placed successfully.";
    return true;
}

std::vector<Order> Store::getCustomerOrders(int customerId) const {
    std::vector<Order> mine;
    for (const Order& order : orders) {
        if (order.getCustomerId() == customerId) mine.push_back(order);
    }
    return mine;
}

int Store::getProductStock(int productId) const {
    const Product* product = findProductById(productId);
    return (product != nullptr) ? product->getStock() : -1;
}

// =====================================================================
// Shopping cart and order screens
// =====================================================================

// Reads a number. Returns false only when the input has ended; valid is false for non-numbers.
bool Store::readNumber(const std::string& prompt, int& value, bool& valid) {
    std::string line;
    if (!readLine(prompt, line)) return false;
    valid = parseNumber(line, value);
    return true;
}

void Store::showCart() const {
    std::cout << "\n" << LINE << "\n"
              << "             SHOPPING CART\n"
              << LINE << "\n";
    std::vector<CartItem> cart = getCart(loggedInCustomerId);
    if (cart.empty()) {
        std::cout << "\n[INFO] Your cart is empty.\n";
        return;
    }
    std::cout << "\n" << std::left
              << std::setw(6) << "ID" << std::setw(30) << "Product" << std::setw(16) << "Price"
              << std::setw(6) << "Qty" << "Subtotal\n"
              << std::string(76, '-') << "\n";
    std::vector<std::string> warnings;
    for (const CartItem& item : cart) {
        const Product* product = findProductById(item.getProductId());
        if (product == nullptr) continue;
        std::cout << std::left << std::setw(6) << product->getId()
                  << std::setw(30) << shorten(product->getName(), 28)
                  << std::setw(16) << formatPaise(product->getPricePaise())
                  << std::setw(6) << item.getQuantity()
                  << formatPaise(product->getPricePaise() * item.getQuantity()) << "\n";
        if (item.getQuantity() > product->getStock()) {
            warnings.push_back(product->getName() + ": only " + std::to_string(product->getStock()) +
                               " in stock now. Please update the quantity.");
        }
    }
    std::cout << std::string(76, '-') << "\n"
              << std::right << std::setw(76) << ("TOTAL: " + formatPaise(getCartTotalPaise(loggedInCustomerId)))
              << std::left << "\n";
    for (const std::string& warning : warnings) std::cout << "[WARNING] " << warning << "\n";
}

void Store::cartAddScreen() {
    showAllProducts();
    if (products.empty()) return;
    int id = 0, quantity = 0;
    bool valid = false;
    if (!readNumber("\nEnter Product ID to add (0 to cancel): ", id, valid)) return;
    if (!valid) { std::cout << "[ERROR] Please enter a valid Product ID (a number).\n"; return; }
    if (id == 0) return;
    if (findProductById(id) == nullptr) { std::cout << "[ERROR] No product found with ID " << id << ".\n"; return; }
    if (!readNumber("Enter quantity: ", quantity, valid)) return;
    if (!valid) { std::cout << "[ERROR] Quantity must be a whole number greater than zero (digits only).\n"; return; }

    std::string message;
    if (addToCart(loggedInCustomerId, id, quantity, message)) {
        std::cout << "\n[SUCCESS] " << message << "\n";
        showCart();
    } else {
        std::cout << "[ERROR] " << message << "\n";
    }
}

void Store::cartUpdateScreen() {
    if (getCart(loggedInCustomerId).empty()) { std::cout << "\n[INFO] Your cart is empty. Add a product first.\n"; return; }
    int id = 0, quantity = 0;
    bool valid = false;
    if (!readNumber("\nEnter Product ID to update (0 to cancel): ", id, valid)) return;
    if (!valid) { std::cout << "[ERROR] Please enter a valid Product ID (a number).\n"; return; }
    if (id == 0) return;
    if (!readNumber("Enter new quantity: ", quantity, valid)) return;
    if (!valid) { std::cout << "[ERROR] Quantity must be a whole number greater than zero (digits only).\n"; return; }

    std::string message;
    if (updateCartQuantity(loggedInCustomerId, id, quantity, message)) {
        std::cout << "\n[SUCCESS] " << message << "\n";
        showCart();
    } else {
        std::cout << "[ERROR] " << message << "\n";
    }
}

void Store::cartRemoveScreen() {
    if (getCart(loggedInCustomerId).empty()) { std::cout << "\n[INFO] Your cart is empty. Nothing to remove.\n"; return; }
    int id = 0;
    bool valid = false;
    if (!readNumber("\nEnter Product ID to remove (0 to cancel): ", id, valid)) return;
    if (!valid) { std::cout << "[ERROR] Please enter a valid Product ID (a number).\n"; return; }
    if (id == 0) return;

    std::string message;
    if (removeFromCart(loggedInCustomerId, id, message)) {
        std::cout << "\n[SUCCESS] " << message << "\n";
        showCart();
    } else {
        std::cout << "[ERROR] " << message << "\n";
    }
}

void Store::cartScreen() {
    if (!requireLogin()) return;
    showCart();
    while (isLoggedIn() && !inputClosed) {
        std::cout << "\n1. Add Product\n"
                  << "2. Update Quantity\n"
                  << "3. Remove Product\n"
                  << "4. View Cart\n"
                  << "5. Proceed to Checkout\n"
                  << "0. Back\n\n"
                  << "Enter your choice: ";
        int choice = -1;
        if (!readChoice(choice)) return;
        switch (choice) {
            case 1: cartAddScreen(); break;
            case 2: cartUpdateScreen(); break;
            case 3: cartRemoveScreen(); break;
            case 4: showCart(); break;
            case 5:
                if (checkoutScreen()) return;     // order placed: the cart is now empty
                break;
            case 0: return;
            default:
                std::cout << "\n[ERROR] Invalid choice. Please enter a number from 0 to 5.\n";
                break;
        }
    }
}

// Returns true only if an order was placed.
bool Store::checkoutScreen() {
    if (!requireLogin()) return false;

    // 1. Empty cart? 2. Is there enough stock right now?
    std::vector<std::string> problems = getCartProblems(loggedInCustomerId);
    if (!problems.empty()) {
        std::cout << "\n[ERROR] Checkout is not possible:\n";
        for (const std::string& problem : problems) std::cout << "        - " << problem << "\n";
        if (getCart(loggedInCustomerId).empty()) std::cout << "        Add products from the Shopping Cart menu first.\n";
        else std::cout << "        Please update your cart and try again. Nothing was ordered.\n";
        return false;
    }

    // 3. Order summary
    const Customer* customer = findCustomerById(loggedInCustomerId);
    std::cout << "\n" << LINE << "\n"
              << "              ORDER SUMMARY\n"
              << LINE << "\n\n"
              << "Customer Name: " << customer->getName() << "\n\n"
              << std::left << std::setw(36) << "Product" << std::setw(10) << "Quantity" << "Amount\n"
              << std::string(60, '-') << "\n";
    for (const CartItem& item : getCart(loggedInCustomerId)) {
        const Product* product = findProductById(item.getProductId());
        std::cout << std::left << std::setw(36) << shorten(product->getName(), 34)
                  << std::setw(10) << item.getQuantity()
                  << formatPaise(product->getPricePaise() * item.getQuantity()) << "\n";
    }
    std::cout << std::string(60, '-') << "\n"
              << "TOTAL AMOUNT: " << formatPaise(getCartTotalPaise(loggedInCustomerId)) << "\n";

    // 4. Confirm or cancel (anything else asks again; after MAX_ATTEMPTS the order is cancelled)
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        std::cout << "\n1. Confirm Order\n0. Cancel Order\n\nEnter your choice: ";
        int choice = -1;
        if (!readChoice(choice)) {
            std::cout << "\n[INFO] Input closed. The order was not placed.\n";
            return false;
        }
        if (choice == 0) {
            std::cout << "\n[INFO] Order cancelled. Your cart and the stock were not changed.\n";
            return false;
        }
        if (choice == 1) {
            std::string message;
            int orderId = 0;
            long long totalPaise = getCartTotalPaise(loggedInCustomerId);
            if (!placeOrder(loggedInCustomerId, message, orderId)) {
                std::cout << "\n[ERROR] Order not placed: " << message << "\n"
                          << "        Nothing was ordered and the stock was not changed.\n";
                return false;
            }
            std::cout << "\n" << LINE << "\n"
                      << "           ORDER CONFIRMED\n"
                      << LINE << "\n"
                      << "[SUCCESS] Thank you, " << customer->getName() << "! Your order has been placed.\n"
                      << "Order ID     : " << orderId << "\n"
                      << "Total Amount : " << formatPaise(totalPaise) << "\n"
                      << "Status       : Pending\n"
                      << THIN << "\n"
                      << "Your confirmed order is saved permanently. Shopping carts are session-based.\n";
            return true;
        }
        std::cout << "\n[ERROR] Invalid choice. Please enter 1 to confirm or 0 to cancel.\n";
    }
    std::cout << "\n[INFO] Too many invalid entries. Order cancelled. Your cart and the stock were not changed.\n";
    return false;
}

void Store::showOrderDetails(const Order& order) const {
    std::cout << "\n" << LINE << "\n"
              << "             ORDER DETAILS\n"
              << LINE << "\n"
              << "Order ID : " << order.getOrderId() << "\n"
              << "Date     : " << order.getOrderDate() << "\n"
              << "Status   : " << order.getStatus() << "\n\n"
              << std::left << std::setw(6) << "ID" << std::setw(30) << "Product" << std::setw(16) << "Price"
              << std::setw(6) << "Qty" << "Subtotal\n"
              << std::string(76, '-') << "\n";
    for (const OrderItem& item : order.getItems()) {
        std::cout << std::left << std::setw(6) << item.getProductId()
                  << std::setw(30) << shorten(item.getProductName(), 28)
                  << std::setw(16) << formatPaise(item.getUnitPricePaise())
                  << std::setw(6) << item.getQuantity()
                  << formatPaise(item.getSubtotalPaise()) << "\n";
    }
    std::cout << std::string(76, '-') << "\n"
              << std::right << std::setw(76) << ("TOTAL: " + formatPaise(order.getTotalPaise()))
              << std::left << "\n";
}

void Store::viewOrdersScreen() {
    if (!requireLogin()) return;
    std::cout << "\n" << LINE << "\n"
              << "              MY ORDERS\n"
              << LINE << "\n";

    // Only orders with this customer's ID are ever collected here
    std::vector<Order> mine = getCustomerOrders(loggedInCustomerId);
    if (mine.empty()) {
        std::cout << "\n[INFO] You have not placed any orders yet.\n";
        return;
    }
    std::cout << "\n" << std::left << std::setw(10) << "Order ID" << std::setw(22) << "Date"
              << std::setw(8) << "Items" << std::setw(18) << "Total" << "Status\n"
              << std::string(66, '-') << "\n";
    for (const Order& order : mine) {
        std::cout << std::left << std::setw(10) << order.getOrderId()
                  << std::setw(22) << order.getOrderDate()
                  << std::setw(8) << order.getItems().size()
                  << std::setw(18) << formatPaise(order.getTotalPaise())
                  << order.getStatus() << "\n";
    }
    std::cout << std::string(66, '-') << "\n";

    while (!inputClosed) {
        int id = 0;
        bool valid = false;
        if (!readNumber("\nEnter Order ID for details (0 to go back): ", id, valid)) return;
        if (!valid) { std::cout << "[ERROR] Please enter a valid Order ID (a number).\n"; continue; }
        if (id == 0) return;
        const Order* chosen = nullptr;
        for (const Order& order : mine) {
            if (order.getOrderId() == id) chosen = &order;
        }
        if (chosen == nullptr) {
            std::cout << "[ERROR] No order with ID " << id << " was found in your order history.\n";
            continue;
        }
        showOrderDetails(*chosen);
    }
}

// =====================================================================
// Administration and permanent records
// =====================================================================

void Store::saveAllData() {
    bool a=fileManager.saveProducts(products), b=fileManager.saveCustomers(customers), c=fileManager.saveOrders(orders);
    if(a&&b&&c) std::cout << "\n[SUCCESS] All store records saved. Backup files are maintained automatically.\n";
    else std::cout << "\n[ERROR] One or more files could not be saved. Check the data folder and permissions.\n";
}

void Store::adminLoginScreen() {
    if (!fileManager.adminExists()) {
        std::cout << "\n" << LINE << "\n             INITIAL ADMIN SETUP\n" << LINE << "\n"
                  << "Create the first administrator account. Use a password with 8-32 characters,\n"
                  << "at least one letter and one digit. The password is stored as a salted hash.\n\n";
        std::string user, pass, confirm;
        if(!readLine("Admin username : ",user) || user.empty() || user.size()>40) { std::cout<<"[ERROR] Invalid username.\n"; return; }
        if(!promptValidated("Admin password : ",validatePassword,pass)) return;
        if(!readLine("Confirm password: ",confirm) || pass!=confirm) { std::cout<<"[ERROR] Passwords do not match.\n"; return; }
        try { if(!fileManager.saveAdmin(user,PasswordUtils::hashPassword(pass))) {std::cout<<"[ERROR] Could not save administrator record.\n";return;} }
        catch(const std::exception&) {std::cout<<"[ERROR] Secure password setup failed.\n";return;}
        std::cout<<"[SUCCESS] Administrator account created. Please log in.\n";
    }
    std::string user,pass;
    if(!readLine("Admin username: ",user)||!readLine("Admin password: ",pass))return;
    if(!fileManager.verifyAdmin(user,pass)){std::cout<<"\n[ERROR] Incorrect administrator credentials.\n";return;}
    adminLoggedIn=true; std::cout<<"\n[SUCCESS] Administrator login successful.\n"; adminPanel();
    adminLoggedIn=false;
}

void Store::adminProducts(int action) {
    while(!inputClosed && adminLoggedIn) {
        int ch=action;
        if(ch==0){std::cout<<"\n"<<LINE<<"\n             PRODUCT MANAGEMENT\n"<<LINE<<"\n1. View Products\n2. Add Product\n3. Update Product\n4. Delete Product\n5. Search Product\n0. Back\nChoice: ";if(!readChoice(ch))return;}
        if(ch==0)return;
        if(ch==1)showAllProducts();
        else if(ch==2){
            int id,stock;std::string name,cat,desc,priceText;double price;
            bool valid=false;if(!readNumber("Product ID: ",id,valid)||!valid)continue;
            if(!readLine("Name: ",name)||!readLine("Category: ",cat)||!readLine("Description: ",desc))return;
            std::string priceStr;if(!readLine("Price (rupees): ",priceStr))return;
            try{price=std::stod(priceStr);}catch(...){std::cout<<"[ERROR] Invalid price.\n";continue;}
            std::string st;if(!readLine("Stock quantity: ",st))return;try{size_t pos=0;stock=std::stoi(st,&pos);if(pos!=st.size())throw std::runtime_error("bad");}catch(...){std::cout<<"[ERROR] Invalid stock.\n";continue;}
            try{Product p(id,name,cat,price,stock,desc);std::string err;if(addProduct(p,err)){fileManager.saveProducts(products);std::cout<<"[SUCCESS] Product added and saved.\n";}else std::cout<<"[ERROR] "<<err<<"\n";}catch(const std::exception&e){std::cout<<"[ERROR] "<<e.what()<<"\n";}
        } else if(ch==3){
            int id;bool valid=false;if(!readNumber("Product ID to update: ",id,valid)||!valid)continue;Product*p=findProductById(id);if(!p){std::cout<<"[ERROR] Product not found.\n";continue;}
            Product candidate=*p; std::string v;
            if(!readLine("New name (blank keeps current): ",v))return;
            if(!v.empty()&&!candidate.setName(v)){std::cout<<"[ERROR] Invalid name.\n";continue;}
            if(!readLine("New category (blank keeps current): ",v))return;
            if(!v.empty()&&!candidate.setCategory(v)){std::cout<<"[ERROR] Invalid category.\n";continue;}
            if(!readLine("New description (blank keeps current): ",v))return;
            if(!v.empty())candidate.setDescription(v);
            if(!readLine("New price in rupees (blank keeps current): ",v))return;
            if(!v.empty()){try{size_t z=0;double x=std::stod(v,&z);if(z!=v.size()||!candidate.setPrice(x))throw std::runtime_error("invalid");}catch(...){std::cout<<"[ERROR] Invalid price; update not saved.\n";continue;}}
            if(!readLine("New stock (blank keeps current): ",v))return;
            if(!v.empty()){try{size_t z=0;int x=std::stoi(v,&z);if(z!=v.size()||!candidate.setStock(x))throw std::runtime_error("invalid");}catch(...){std::cout<<"[ERROR] Invalid stock; update not saved.\n";continue;}}
            *p=candidate;
            fileManager.saveProducts(products);std::cout<<"[SUCCESS] Product updated.\n";
        } else if(ch==4){
            int id;bool valid=false;if(!readNumber("Product ID to delete: ",id,valid)||!valid)continue;bool referenced=false;for(const auto&o:orders)for(const auto&i:o.getItems())if(i.getProductId()==id)referenced=true;
            if(referenced){std::cout<<"[ERROR] This product appears in order history and cannot be deleted. Set its stock to zero instead.\n";continue;}
            auto it=products.begin();while(it!=products.end()&&it->getId()!=id)++it;if(it==products.end())std::cout<<"[ERROR] Product not found.\n";else{products.erase(it);fileManager.saveProducts(products);std::cout<<"[SUCCESS] Product deleted.\n";}
        } else if(ch==5){std::string term;if(!readLine("Search name: ",term))return;searchByName(term);}
        else std::cout<<"[ERROR] Invalid choice.\n";
        if(action!=0)return;
    }
}

void Store::adminCustomers() const {
    std::cout<<"\n"<<LINE<<"\n             REGISTERED CUSTOMERS\n"<<LINE<<"\n";
    if(customers.empty()){std::cout<<"No customers registered.\n";return;}
    for(const Customer&c:customers){int count=0;for(const Order&o:orders)if(o.getCustomerId()==c.getId())++count;
        std::cout<<"ID: "<<c.getId()<<" | Name: "<<c.getName()<<" | Email: "<<c.getEmail()<<" | Phone: "<<c.getPhone()<<" | Orders: "<<count<<"\n";}
    std::cout<<"\nPassword information is never displayed.\n";
}

void Store::adminOrders() {
    std::cout<<"\n"<<LINE<<"\n                ALL ORDERS\n"<<LINE<<"\n";
    if(orders.empty()){std::cout<<"No orders found.\n";return;}
    for(const Order&o:orders){const Customer*c=findCustomerById(o.getCustomerId());std::cout<<"\nOrder #"<<o.getOrderId()<<" | Customer: "<<(c?c->getName():"Unknown")<<" ("<<o.getCustomerId()<<") | Date: "<<o.getOrderDate()<<" | Status: "<<o.getStatus()<<" | Total: "<<formatPaise(o.getTotalPaise())<<"\n";showOrderDetails(o);}
}

void Store::adminStatistics() const {
    int pending=0,delivered=0,units=0;long long value=0,revenue=0;
    for(const Product&p:products)units+=p.getStock();
    for(const Order&o:orders){value+=o.getTotalPaise();if(o.getStatus()=="Pending")++pending;if(o.getStatus()=="Delivered")++delivered;if(o.getStatus()!="Cancelled")revenue+=o.getTotalPaise();}
    std::cout<<"\n"<<LINE<<"\n              STORE STATISTICS\n"<<LINE<<"\n"
             <<"Products: "<<products.size()<<"\nCustomers: "<<customers.size()<<"\nOrders: "<<orders.size()<<"\nPending orders: "<<pending<<"\nDelivered orders: "<<delivered<<"\nUnits in stock: "<<units<<"\nTotal order value (including cancelled): "<<formatPaise(value)<<"\nNon-cancelled order value (not payment-confirmed revenue): "<<formatPaise(revenue)<<"\n";
}

void Store::adminPanel() {
    while(adminLoggedIn&&!inputClosed){
        std::cout<<"\n"<<LINE<<"\n              ADMIN DASHBOARD\n"<<LINE<<"\n1. View All Products\n2. Add New Product\n3. Update Product\n4. Delete Product\n5. View All Customers\n6. View All Orders\n7. Update Order Status\n8. View Store Statistics\n9. Save Data\n10. Logout\nChoice: ";
        int ch=-1;if(!readChoice(ch))return;
        switch(ch){case 1:showAllProducts();break;case 2:adminProducts(2);break;case 3:adminProducts(3);break;case 4:adminProducts(4);break;case 5:adminCustomers();break;case 6:adminOrders();break;case 7:{int id;bool ok=false;if(!readNumber("Order ID: ",id,ok)||!ok)break;Order* target=nullptr;for(Order&o:orders)if(o.getOrderId()==id)target=&o;if(!target){std::cout<<"Order not found.\n";break;}std::string st;if(!readLine("Status (Pending/Confirmed/Shipped/Delivered/Cancelled): ",st))return;const std::string allowed[]={"Pending","Confirmed","Shipped","Delivered","Cancelled"};bool good=false;for(const auto&a:allowed)if(st==a)good=true;if(!good){std::cout<<"Invalid status.\n";break;}target->setStatus(st);fileManager.saveOrders(orders);std::cout<<"Order status updated.\n";break;}
        case 8:adminStatistics();break;case 9:saveAllData();break;case 10:adminLoggedIn=false;std::cout<<"Logged out.\n";return;default:std::cout<<"Invalid choice.\n";}
    }
}

// =====================================================================
// Main menu
// =====================================================================

void Store::displayMainMenu() const {
    std::cout << "\n" << LINE << "\n"
              << "          TECHSTORE MANAGEMENT\n"
              << LINE << "\n\n"
              << "1. About TechStore\n"
              << "2. View Products\n"
              << "3. Search Products\n"
              << "4. Customer Registration\n"
              << "5. Customer Login\n"
              << "6. Shopping Cart\n"
              << "7. Place Order\n"
              << "8. View My Orders\n"
              << "9. Admin Login\n"
              << "0. Exit\n\n"
              << "Enter your choice: ";
}

void Store::showAbout() const {
    std::cout << "\n" << LINE << "\n"
              << "            ABOUT TECHSTORE\n"
              << LINE << "\n\n"
              << "TechStore is a terminal-based e-commerce management\n"
              << "system written in C++17.\n\n"
              << "Customers can:\n"
              << "  - Browse and search tech products\n"
              << "  - Register and log in\n"
              << "  - Add products to a shopping cart\n"
              << "  - Place orders and view order history\n\n"
              << "Administrators can:\n"
              << "  - Manage products, customers and orders\n\n"
              << THIN << "\n"
              << "Version : 4.0\n"
              << "Language: C++17 (OOP + File Handling)\n"
              << THIN << "\n";
}

void Store::handleChoice(int choice, bool& running) {
    switch (choice) {
        case 1: showAbout(); break;
        case 2: viewProductsScreen(); break;
        case 3: searchScreen(); break;
        case 4: registrationScreen(); break;
        case 5: loginScreen(); break;
        case 6: cartScreen(); break;
        case 7: checkoutScreen(); break;
        case 8: viewOrdersScreen(); break;
        case 9: adminLoginScreen(); break;
        case 0:
            std::cout << "\n" << LINE << "\n"
                      << "  Thank you for using TechStore. Goodbye!\n"
                      << LINE << "\n";
            running = false;
            break;
        default:
            std::cout << "\n[ERROR] Invalid choice. Please enter a number from 0 to 9.\n";
            break;
    }
}

void Store::run() {
    bool running = true;
    while (running && !inputClosed) {
        displayMainMenu();
        int choice = -1;
        if (!readChoice(choice)) break;
        handleChoice(choice, running);
    }
    if (inputClosed) {
        std::cout << "\n\n[INFO] Input closed. Exiting TechStore.\n";
    }
}
