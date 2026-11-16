# TechStore – E-Commerce Management System

## Student Details

| Details | Information |
|---|---|
| **Student Name** | Shubham Narsale |
| **Roll Number** | AD2139 |
| **College** | Zeal College of Engineering and Research, Pune |
| **Project Title** | TechStore – E-Commerce Management System |
| **Programming Language** | C++17 |
| **Project Type** | Menu-driven console application |

---

## 1. Project Abstract

TechStore is a menu-driven e-commerce management application developed in C++17 using object-oriented programming. It allows customers to browse and search technology products, create an account, log in, manage a shopping cart, place orders and view their order history. An administrator can manage the product catalogue, view registered customers and orders, update order statuses and view store statistics.

The application stores product, customer, order and administrator records in structured local files inside the `data/` directory. Customer and administrator passwords are stored as salted PBKDF2-HMAC-SHA256 password records rather than plain-text passwords. The project demonstrates classes, encapsulation, vectors, maps, input validation, exception handling, file handling and modular programming.

## 2. Problem Statement

Managing products, customers and orders manually can be time-consuming and may lead to incorrect records, stock errors and difficulty tracking orders. A simple computer-based system is required to organise product information, maintain customer accounts, validate shopping-cart quantities, process orders and preserve records between application sessions.

TechStore addresses this problem through a menu-driven C++ application with separate customer and administrator operations and local file-based data storage.

## 3. Objectives

1. Develop a menu-driven e-commerce management application using C++17.
2. Apply object-oriented programming concepts through classes and encapsulation.
3. Allow customers to register, log in, browse products and search by name or category.
4. Provide shopping-cart operations to add, update, remove and view products.
5. Validate product stock before confirming an order.
6. Allow customers to view their account information and order history.
7. Provide administrator functions for product, customer and order management.
8. Store application records in local files so that important data remains available after restarting the program.
9. Validate user input and protect stored passwords using salted password hashing.
10. Test the main data, cart, order and persistence operations.

## 4. Main Features

### Customer Features

- Customer registration with name, email, password and phone number.
- Customer login and logout.
- View the product catalogue and individual product details.
- Search products by name or category.
- View customer account information.
- Add products to the shopping cart.
- Update product quantities and remove cart items.
- Check available stock before checkout.
- Confirm or cancel an order.
- View personal order history and order details.

### Administrator Features

- Create the first administrator account during initial setup.
- Log in to the administrator dashboard.
- View all products.
- Add new products.
- Update product information.
- Delete products.
- View registered customers.
- View all customer orders and their details.
- Update order status to Pending, Confirmed, Shipped, Delivered or Cancelled.
- View store statistics.
- Save application data manually.

### Reliability and Security

- Customer and administrator passwords are stored as salted PBKDF2-HMAC-SHA256 records; plain-text passwords are not stored.
- Money totals are calculated in whole paise to reduce floating-point rounding errors.
- Product, stock, quantity and account inputs are validated.
- Product, customer, order and administrator data use structured local files with format/version information.
- Backup copies with the `.bak` extension are maintained when files are replaced successfully.
- Order items retain the product name and unit price recorded when the order was placed.
- Shopping carts are kept in memory for the current application session and are cleared when the application closes.

## 5. Project Folder Structure

```text
TechStore_CPP/
├── main.cpp
├── models.h
├── store.h
├── store.cpp
├── file_manager.h
├── file_manager.cpp
├── password_utils.h
├── password_utils.cpp
├── build.bat
├── README.md
├── .gitignore
└── tests/
    ├── test_units.cpp
    ├── test_orders.cpp
    ├── test_persistence.cpp
    ├── functional_tests.sh
    ├── run_tests.bat
    └── TEST_RESULTS.txt
```

The `data/` directory is created by the application when it needs to save records. It is excluded from Git by `.gitignore` because it contains local application data and account records.

## 6. Software and Hardware Requirements

### Software Requirements

- Operating System: Windows 10/11 (the supplied Windows build script is intended for MSYS2 UCRT64).
- Programming Language: C++17.
- Compiler: GCC provided by MSYS2 UCRT64.
- Code Editor: Visual Studio Code (recommended).
- VS Code Extension: C/C++ by Microsoft (recommended for editing and IntelliSense).

### Hardware Requirements

- A desktop computer or laptop.
- Keyboard and monitor/display.
- Sufficient free storage for the source files, compiler and application data.

## 7. How to Compile and Run in VS Code

### One-time setup

1. Install Visual Studio Code.
2. Install MSYS2 and configure the **UCRT64** environment.
3. Install the UCRT64 GCC compiler and ensure `g++` is available in the terminal PATH.
4. Open a terminal and check the compiler:

```powershell
g++ --version
```

### Run the project

1. Extract the project ZIP file.
2. In VS Code, select **File → Open Folder** and open the `TechStore_CPP` folder.
3. Open the integrated terminal using **Terminal → New Terminal**.
4. Use an MSYS2 UCRT64 terminal environment with the required GCC compiler and Windows BCrypt library support.
5. Build the application:

```powershell
.\build.bat
```

6. If the build is successful, run the application:

```powershell
.\techstore.exe
```

**Note:** In Windows PowerShell, commands in the current directory are run with the `./` or ` .\` prefix. Therefore, use ` .\build.bat` rather than `build.bat`.

The build script compiles the source files with the C++17 standard and links the Windows BCrypt library using `-lbcrypt`.

## 8. Main Menu and Sample Program Output

When the application starts, it displays the following main menu:

```text
------------------------------------------------------------
                  TECHSTORE MANAGEMENT
------------------------------------------------------------

1. About TechStore
2. View Products
3. Search Products
4. Customer Registration
5. Customer Login
6. Shopping Cart
7. Place Order
8. View My Orders
9. Admin Login
0. Exit

Enter your choice:
```

### Sample customer workflow

The following is an illustrative example of a customer session. The exact product list, generated customer ID, order ID, date and totals depend on the application's current data and the actions performed.

```text
Enter your choice: 4

                     CUSTOMER REGISTRATION

Full Name: Rahul Sharma
Email Address: rahul@example.com
Password: Rahul123
Confirm Password: Rahul123
Phone Number: 9876543210

[SUCCESS] Registration successful.

Enter your choice: 5

                         CUSTOMER LOGIN

Email: rahul@example.com
Password: Rahul123

[SUCCESS] Login successful.

                    CUSTOMER PANEL

1. View Products
2. Search Products
3. View My Account
4. Shopping Cart
5. Place Order
6. View My Orders
0. Logout

Enter your choice: 3

                       MY ACCOUNT

Customer ID : [Generated customer ID]
Name        : Rahul Sharma
Email       : rahul@example.com
Phone       : 9876543210
```

### Shopping cart options

The shopping-cart menu provides these operations:

```text
1. Add Product
2. Update Quantity
3. Remove Product
4. View Cart
5. Proceed to Checkout
0. Back
```

At checkout, the customer can confirm or cancel the order:

```text
1. Confirm Order
0. Cancel Order
```

The application does not implement a payment gateway or payment-method selection. An order is placed when the customer confirms the order.

### Administrator dashboard

After administrator setup and login, the dashboard provides:

```text
1. View All Products
2. Add New Product
3. Update Product
4. Delete Product
5. View All Customers
6. View All Orders
7. Update Order Status
8. View Store Statistics
9. Save Data
10. Logout
```

## 9. Object-Oriented Concepts Used

| Concept / Class | Explanation |
|---|---|
| `Product` | Represents a product with ID, name, category, price, stock and description. It validates product values. |
| `Customer` | Represents a registered customer and stores account information with a password hash record. |
| `CartItem` | Represents a product ID and quantity in a customer's in-memory cart. |
| `OrderItem` | Stores product details, quantity and unit price as recorded at the time of purchase. |
| `Order` | Represents a customer's order, including order ID, customer ID, items, date and status. |
| `Store` | Manages the application data, menus, customer operations, cart operations and order processing. |
| `FileManager` | Loads and saves product, customer, order and administrator records. |
| `PasswordUtils` | Provides password hashing and password verification functions. |

### Encapsulation

Class data members are kept private where appropriate. Public member functions provide controlled access to the data and validate values before accepting changes.

### Constructors and Member Functions

The model classes use constructors to initialise objects. Getters provide access to stored values, while setters validate and update selected fields.

### Vectors and Maps

Vectors store products, customers, orders and order items. A map associates each customer ID with that customer's cart during the current session.

### File Handling

The `FileManager` class handles loading and saving structured records in local files. This allows product, customer, order and administrator records to be available when the application is started again.

## 10. Data Storage

The application creates a `data/` directory in its working directory and uses these files:

| File | Purpose |
|---|---|
| `products.dat` | Stores the product catalogue. |
| `customers.dat` | Stores customer details and password records. |
| `orders.dat` | Stores order headers and order items. |
| `admin.dat` | Stores the administrator account record. |
| `*.bak` | Backup copies created when a data file is successfully replaced. |

The `data/` directory is local application data and is ignored by Git. Do not upload real customer records, password records or private backup files to a public repository.

## 11. Testing

The project includes automated test programs for password/model utilities, shopping-cart and order operations, and file persistence.

### Run tests on Windows

Open the MSYS2 UCRT64 terminal in the project folder and run:

```bat
tests\run_tests.bat
```

### Run functional tests on Linux

```bash
bash tests/functional_tests.sh
```

The included `tests/TEST_RESULTS.txt` records the test results available when this project package was prepared. Test results may differ if the source code or environment is changed. The Windows build and Windows BCrypt implementation should be tested on the target Windows computer.

## 12. Advantages

1. Provides a simple menu-driven interface.
2. Separates customer and administrator operations.
3. Organises product, customer and order information.
4. Checks stock availability before placing an order.
5. Keeps order details and purchase-time prices in the order record.
6. Saves important records in local files.
7. Uses input validation and password hashing.
8. Demonstrates important C++ object-oriented programming concepts.

## 13. Future Scope

The project can be extended with a graphical user interface, a database, online payment integration, product images, delivery tracking, email notifications and role-based access controls. These are possible future improvements and are not part of the current console application.

## 14. Conclusion

TechStore – E-Commerce Management System provides customer and administrator functions through a C++17 console application. It supports product browsing and searching, customer registration and login, shopping-cart management, order placement, order history and administrator operations. Local file handling preserves important records, while validation and password hashing improve reliability and account security. The project provides practical experience with object-oriented programming, standard library containers, input validation and file handling in C++.

---

**Prepared by:** Shubham Narsale  
**Roll Number:** AD2139  
**Zeal College of Engineering and Research, Pune**
