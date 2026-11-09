
const API_URL = "http://localhost:8080/api";

let currentDriver = null;
let currentTruck = null;

// Fetch data from the C++ backend
async function apiRequest(endpoint) {
    const response = await fetch(API_URL + endpoint);

    if (!response.ok) {
        throw new Error("API request failed: " + endpoint);
    }

    return await response.json();
}

// Get HTML elements easily
function getElement(id) {
    return document.getElementById(id);
}

// Display messages
function showMessage(message, type) {
    const element = getElement("driverMessage");

    element.textContent = message;
    element.className = "message " + type;
}

// Format capacity in kg
function formatKG(value) {
    return Number(value).toFixed(1) + " kg";
}

// Find and load the registered driver
async function lookupDriver() {
    const input = getElement("driverIdInput");

    const driverID = input.value.trim().toUpperCase();

    if (!/^D[0-9]+$/.test(driverID)) {
        showMessage("Enter a valid Driver ID, such as D101.", "error");
        return;
    }

    const button = getElement("lookupDriverBtn");

    button.disabled = true;
    button.textContent = "Checking...";

    try {
        // Get registered drivers from SQLite
        const drivers = await apiRequest("/drivers");

        currentDriver = drivers.find(function (driver) {
            return driver.id.toUpperCase() === driverID;
        });

        if (!currentDriver) {
            currentDriver = null;
            currentTruck = null;

            getElement("driverWorkspace").hidden = true;

            showMessage(
                "Driver ID not found. Please contact the administrator.",
                "error"
            );

            return;
        }

        // Load the driver's assigned truck
        currentTruck = null;

        if (currentDriver.truckId) {
            const trucks = await apiRequest("/trucks");

            currentTruck = trucks.find(function (truck) {
                return truck.id === currentDriver.truckId;
            }) || null;
        }
        sessionStorage.setItem(
    "smartwasteDriverId",
    currentDriver.id
);

        displayDriverProfile();
        displayTruckDetails();

        getElement("driverWorkspace").hidden = false;

        showMessage(
            "Driver information loaded successfully.",
            "success"
        );

    } catch (error) {
        console.error("Driver lookup error:", error);

        showMessage(
            "Could not connect to SmartWaste API. Check whether the server is running.",
            "error"
        );

    } finally {
        button.disabled = false;
        button.textContent = "Access Dashboard";
    }
}

// Display driver information
function displayDriverProfile() {
    getElement("sidebarDriverName").textContent =
        currentDriver.name;

    getElement("driverId").textContent =
        currentDriver.id;

    getElement("driverName").textContent =
        currentDriver.name;

    getElement("driverPhone").textContent =
        currentDriver.phone || "Not available";

    getElement("truckId").textContent =
        currentDriver.truckId || "Not assigned";
}

// Display truck capacity and status
function displayTruckDetails() {
    if (!currentTruck) {
        getElement("truckCapacity").textContent = "--";
        getElement("truckLoad").textContent = "--";
        getElement("truckRemaining").textContent = "--";
        getElement("truckStatus").textContent = "No Truck";

        getElement("truckUtilization").textContent = "0%";
        getElement("truckProgress").style.width = "0%";

        getElement("truckAlert").textContent =
            "No truck is currently assigned to your driver profile.";

        getElement("truckAlert").className =
            "alert warning";

        return;
    }

    const capacity = Number(currentTruck.capacity);
    const load = Number(currentTruck.load);

    const remaining = Math.max(0, capacity - load);

    const utilization = capacity > 0
        ? (load / capacity) * 100
        : 0;

    getElement("truckCapacity").textContent =
        formatKG(capacity);

    getElement("truckLoad").textContent =
        formatKG(load);

    getElement("truckRemaining").textContent =
        formatKG(remaining);

    getElement("truckStatus").textContent =
        currentTruck.status;

    getElement("truckUtilization").textContent =
        utilization.toFixed(1) + "%";

    getElement("truckProgress").style.width =
        Math.min(utilization, 100) + "%";

    // Apply the 90% unloading rule
    if (utilization >= 90 ||
        currentTruck.status === "NeedsDepot") {

        getElement("truckAlert").textContent =
            "Warning: Truck has reached the 90% unloading threshold. Depot unloading is required.";

        getElement("truckAlert").className =
            "alert danger";

        getElement("truckProgress").style.background =
            "#d96b67";

    } else {
        getElement("truckAlert").textContent =
            "Truck capacity is within the operating limit.";

        getElement("truckAlert").className =
            "alert success";

        getElement("truckProgress").style.background =
            "#29915b";
    }
}

// Assignment details will be connected
// after the active-assignment API is added.
function displayAssignmentPlaceholder() {
    getElement("assignedBin").textContent = "--";
    getElement("binLocation").textContent = "--";
    getElement("binFill").textContent = "--";
    getElement("binGarbage").textContent = "--";

    getElement("assignmentMessage").textContent =
        "Active bin assignment and route details will load after backend assignment integration.";
}

// Start the portal
document.addEventListener("DOMContentLoaded", function () {

    const button = getElement("lookupDriverBtn");
    const input = getElement("driverIdInput");

    button.addEventListener("click", lookupDriver);

    input.addEventListener("keydown", function (event) {
        if (event.key === "Enter") {
            lookupDriver();
        }
    });

    displayAssignmentPlaceholder();
});
