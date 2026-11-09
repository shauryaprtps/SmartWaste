const API_URL = "http://localhost:8080/api";

async function apiRequest(endpoint) {
    const response = await fetch(`${API_URL}${endpoint}`);

    if (!response.ok) {
        throw new Error("Unable to fetch truck information.");
    }

    return await response.json();
}

function showTruckMessage(message, type = "info") {
    const element = document.getElementById("truckMessage");

    element.className = `alert ${type}`;
    element.textContent = message;
}

function refreshTruck() {
    loadTruckDetails();
}

async function loadTruckDetails() {
    const driverID = sessionStorage.getItem("smartwasteDriverId");

    if (!driverID) {
        showTruckMessage(
            "Please open Driver Dashboard and enter your Driver ID first.",
            "warning"
        );
        return;
    }

    try {
        const [drivers, trucks] = await Promise.all([
            apiRequest("/drivers"),
            apiRequest("/trucks")
        ]);

        const driver = drivers.find(
            item => String(item.id) === String(driverID)
        );

        if (!driver) {
            showTruckMessage("Driver record not found.", "error");
            return;
        }

        const truck = trucks.find(
            item => String(item.id) === String(driver.truckId)
        );

        if (!truck) {
            showTruckMessage("No truck is assigned to this driver.", "warning");
            return;
        }

        displayTruckDetails(driver, truck);

        showTruckMessage("Truck information loaded successfully.", "success");

    } catch (error) {
        console.error(error);

        showTruckMessage(
            "Could not connect to the backend. Check whether the API server is running.",
            "error"
        );
    }
}

function displayTruckDetails(driver, truck) {
    const capacity = Number(truck.capacity) || 0;
    const load = Number(truck.load) || 0;

    const remaining = Math.max(0, capacity - load);

    const percentage = capacity > 0
        ? Math.min(100, (load / capacity) * 100)
        : 0;

    document.getElementById("truckId").textContent = truck.id;
    document.getElementById("truckCapacity").textContent =
        `${capacity} kg`;

    document.getElementById("truckLoad").textContent =
        `${load} kg`;

    document.getElementById("truckRemaining").textContent =
        `${remaining} kg`;

    document.getElementById("truckPercentage").textContent =
        `${percentage.toFixed(1)}%`;

    document.getElementById("truckProgress").style.width =
        `${percentage}%`;

    document.getElementById("truckStatus").textContent = truck.status;
    document.getElementById("truckStatusDetails").textContent =
        truck.status;

    document.getElementById("assignedDriver").textContent =
        driver.name;

    document.getElementById("truckLocation").textContent =
        `Node ${truck.location}`;

    const capacityMessage = document.getElementById("capacityMessage");

    if (percentage >= 90) {
        capacityMessage.textContent =
            "Truck capacity is at least 90%. Depot visit required.";
    } else {
        capacityMessage.textContent =
            "Truck has sufficient remaining capacity.";
    }

    displayDepotAlert(percentage);
}

function displayDepotAlert(percentage) {
    const alertBox = document.getElementById("depotAlert");

    if (percentage >= 90) {
        alertBox.textContent =
            "Warning: Truck is 90% or more full. It should be directed to the nearest depot.";
        alertBox.className = "alert warning";
    } else {
        alertBox.textContent =
            "No depot alert. Continue your assigned collection work.";
        alertBox.className = "empty-state";
    }
}

document.addEventListener("DOMContentLoaded", loadTruckDetails);