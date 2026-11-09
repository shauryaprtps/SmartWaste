# SmartWaste
### Smart Garbage Collection & Route Optimization System

SmartWaste is a software-based waste management project designed to improve municipal garbage collection. It combines a C++ backend, Data Structures and Algorithms, SQLite database management, and web-based portals for administrators, drivers, and citizens.

The system focuses on prioritising garbage collection, managing bins and trucks, handling citizen complaints, and supporting efficient collection operations.

---

## Key Features

### Admin Portal
- Manage garbage bins and trucks.
- Manage driver records.
- View and manage citizen complaints.
- Access reports and system information.

### Driver Portal
- View assigned collection work.
- Access truck-related information.
- Update collection-related details through the portal.

### Citizen Portal
- Register and log in as a citizen.
- Submit garbage-related complaints.
- View complaint status.
- Manage profile information.

### Backend and Data Management
- C++ REST API.
- SQLite database integration.
- Role-based authentication.
- Password hashing and session-based authentication.
- Persistent storage of application data.

### Data Structures and Algorithms
- Graph representation for locations and routes.
- Dijkstra's algorithm for shortest-path calculations.
- Heap-based priority management for bins.
- C and C++ implementations of core DSA concepts.

---

## Technology Stack

| Component | Technology |
|---|---|
| Backend | C++ |
| Frontend | HTML, CSS, JavaScript |
| Database | SQLite |
| Data Structures | Graph, Heap, Priority Queue |
| Algorithms | Dijkstra's Shortest Path |
| JSON | nlohmann/json |
| HTTP Server | cpp-httplib |
| Cryptography | OpenSSL |

---

## Project Structure

```text
SmartWaste/
│
├── backend/
│   ├── api.cpp
│   ├── Database.cpp
│   ├── Database.h
│   ├── authapi.cpp
│   ├── authapi.h
│   ├── authmanager.cpp
│   ├── authmanager.h
│   ├── Bin.h
│   ├── CollectionManager.h
│   ├── Depot.h
│   ├── Driver.h
│   ├── Truck.h
│   ├── main.cpp
│   ├── create_admin.cpp
│   └── database_test.cpp
│
├── database/
│   ├── schema.sql
│   └── auth_schema.sql
│
├── dsa/
│   ├── graph.c
│   ├── graph.h
│   ├── heap.c
│   ├── heap.h
│   ├── dijkstra.c
│   ├── dijkstra.h
│   ├── graph_test.c
│   ├── heap_test.c
│   └── dijkstra_test.c
│
├── frontend/
│   ├── admin/
│   ├── citizen/
│   ├── driver/
│   ├── index.html
│   └── login.html
│
├── .gitignore
└── README.md
