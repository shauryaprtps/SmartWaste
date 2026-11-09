
const API_URL = "http://localhost:8080/api";

let trucks = [];
let editingTruckId = null;
let isLoading = false;
let loadError = "";

// Prevent HTML injection
function escapeHTML(value) {
    return String(value).replace(/[&<>"']/g, character => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[character]);
}

// Send requests to C++ API
async function apiRequest(endpoint, options = {}) {
    const response = await fetch(`${API_URL}${endpoint}`, {
        ...options,
        headers: {
            ...(options.body ? {
                "Content-Type": "application/json"
            } : {}),
            ...options.headers
        }
    });

    const result = await response.json().catch(() => ({}));

    if (!response.ok) {
        throw new Error(
            result.message || `Server error: ${response.status}`
        );
    }

    return result;
}

// Determine truck status
function getTruckStatus(truck) {
    if (truck.load >= truck.capacity * 0.90) {
        return "NeedsDepot";
    }

    return truck.status;
}

// Load truck records from SQLite
async function loadTrucks() {
    isLoading = true;
    loadError = "";

    renderTrucks();

    try {
        const result = await apiRequest("/trucks", {
            method: "GET",
            cache: "no-store"
        });

        if (!Array.isArray(result)) {
            throw new Error("Invalid truck data received.");
        }

        trucks = result.map(truck => ({
            id: String(truck.id),
            capacity: Number(truck.capacity),
            load: Number(truck.load),
            location: Number(truck.location),
            status: String(truck.status)
        }));

        isLoading = false;
        renderTrucks();

        return true;

    } catch (error) {
        isLoading = false;
        loadError = error.message;

        renderTrucks();

        console.error("Truck loading error:", error);

        return false;
    }
}

// Refresh button
async function refreshTrucks() {
    await loadTrucks();
}

// Update dashboard statistics
function updateStatistics() {
    document.getElementById("totalTrucks").textContent =
        trucks.length;

    document.getElementById("availableTrucks").textContent =
        trucks.filter(t => getTruckStatus(t) === "Available").length;

    document.getElementById("assignedTrucks").textContent =
        trucks.filter(t => getTruckStatus(t) === "Assigned").length;

    document.getElementById("unloadingTrucks").textContent =
        trucks.filter(t => getTruckStatus(t) === "NeedsDepot").length;
}

// Display truck records
function renderTrucks() {
    const tableBody = document.getElementById("truckTableBody");

    const searchInput = document.getElementById("truckSearch");
    const filterInput = document.getElementById("statusFilter");

    if (!tableBody || !searchInput || !filterInput) return;

    const search = searchInput.value.toLowerCase().trim();
    const filter = filterInput.value;

    if (isLoading) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="8" style="text-align:center;padding:25px;">
                    Loading trucks from SQLite...
                </td>
            </tr>
        `;

        document.getElementById("truckCount").textContent =
            "Connecting to SmartWaste database...";

        return;
    }

    if (loadError) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="8" style="text-align:center;padding:25px;">
                    Unable to load trucks. Check the API server.
                </td>
            </tr>
        `;

        document.getElementById("truckCount").textContent =
            loadError;

        ["totalTrucks", "availableTrucks",
         "assignedTrucks", "unloadingTrucks"].forEach(id => {
            document.getElementById(id).textContent = "--";
        });

        return;
    }

    const filteredTrucks = trucks.filter(truck => {
        const status = getTruckStatus(truck);

        return truck.id.toLowerCase().includes(search) &&
            (filter === "All" || status === filter);
    });

    tableBody.innerHTML = "";

    filteredTrucks.forEach(truck => {
        const status = getTruckStatus(truck);

        const remaining = Math.max(
            0,
            truck.capacity - truck.load
        );

        const percentage = truck.capacity > 0
            ? Math.min(100, Math.max(
                0,
                (truck.load / truck.capacity) * 100
              ))
            : 0;

        const statusLabel = status === "NeedsDepot"
            ? "Needs Unloading"
            : status;

        const row = document.createElement("tr");

        row.innerHTML = `
            <td><strong>${escapeHTML(truck.id)}</strong></td>

            <td>${truck.load} kg</td>

            <td>${truck.capacity} kg</td>

            <td>${remaining} kg</td>

            <td>
                <div class="truck-progress">
                    <div style="width:${percentage}%"></div>
                </div>
                <small>${percentage.toFixed(1)}%</small>
            </td>

            <td>${truck.location}</td>

            <td>
                <span class="truck-status ${status.toLowerCase()}">
                    ${statusLabel}
                </span>
            </td>

            <td>
                <button class="table-action edit-action"
                    data-action="edit"
                    data-truck-id="${escapeHTML(truck.id)}">
                    Edit
                </button>

                <button class="table-action delete-action"
                    data-action="delete"
                    data-truck-id="${escapeHTML(truck.id)}">
                    Delete
                </button>
            </td>
        `;

        tableBody.appendChild(row);
    });

    if (filteredTrucks.length === 0) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="8" style="text-align:center;padding:25px;">
                    No trucks found.
                </td>
            </tr>
        `;
    }

    document.getElementById("truckCount").textContent =
        `Showing ${filteredTrucks.length} of ${trucks.length} trucks. Live data connected to SQLite database.`;

    updateStatistics();
}

// Open add truck modal
function openTruckModal() {
    editingTruckId = null;

    const form = document.getElementById("truckForm");

    form.reset();

    document.getElementById("modalTitle").textContent =
        "Add New Truck";

    document.getElementById("truckId").disabled = false;

    document.getElementById("formError").textContent = "";

    document.getElementById("truckModal").classList.add("show");
}

// Close modal
function closeTruckModal() {
    document.getElementById("truckModal").classList.remove("show");

    document.getElementById("truckForm").reset();

    document.getElementById("truckId").disabled = false;

    document.getElementById("formError").textContent = "";

    editingTruckId = null;
}

// Edit an existing truck
function editTruck(id) {
    const truck = trucks.find(t => t.id === id);

    if (!truck) return;

    editingTruckId = id;

    document.getElementById("modalTitle").textContent =
        "Edit Truck";

    document.getElementById("truckId").value = truck.id;

    document.getElementById("truckId").disabled = true;

    document.getElementById("truckCapacity").value =
        truck.capacity;

    document.getElementById("truckLoad").value =
        truck.load;

    document.getElementById("truckLocation").value =
        truck.location;

    document.getElementById("truckStatus").value =
        getTruckStatus(truck);

    document.getElementById("formError").textContent = "";

    document.getElementById("truckModal").classList.add("show");
}

// Add or update truck in SQLite
document.getElementById("truckForm").addEventListener(
    "submit",
    async function(event) {

        event.preventDefault();

        const id = document.getElementById("truckId")
            .value.trim().toUpperCase();

        const capacity = Number(
            document.getElementById("truckCapacity").value
        );

        const load = Number(
            document.getElementById("truckLoad").value
        );

        const location = Number(
            document.getElementById("truckLocation").value
        );

        let status = document.getElementById("truckStatus").value;

        const error = document.getElementById("formError");

        if (
            !id ||
            !Number.isFinite(capacity) ||
            !Number.isFinite(load) ||
            !Number.isInteger(location) ||
            capacity <= 0 ||
            load < 0 ||
            load > capacity ||
            location < 0 ||
            location > 3
        ) {
            error.textContent =
                "Enter valid truck details. Load cannot exceed capacity and location must be between 0 and 3.";

            return;
        }

        // Automatically apply 90% unloading rule
        if (load >= capacity * 0.90) {
            status = "NeedsDepot";
        }

        const payload = {
            capacity,
            load,
            location,
            status
        };

        let endpoint = "/trucks";
        let method = "POST";

        if (editingTruckId !== null) {
            endpoint = `/trucks/${encodeURIComponent(editingTruckId)}`;
            method = "PUT";
        } else {
            payload.id = id;
        }

        const submitButton = this.querySelector(
            'button[type="submit"]'
        );

        submitButton.disabled = true;

        error.textContent = "Saving truck to SQLite...";

        try {
            await apiRequest(endpoint, {
                method,
                body: JSON.stringify(payload)
            });

            closeTruckModal();

            const loaded = await loadTrucks();

            if (!loaded) {
                alert(
                    "Truck was saved, but refreshing the list failed. Use the Refresh button."
                );
                return;
            }

            alert(
                editingTruckId === null
                    ? "Truck added successfully to SQLite!"
                    : "Truck updated successfully in SQLite!"
            );

        } catch (error) {
            document.getElementById("formError").textContent =
                error.message;

            console.error("Truck save error:", error);

        } finally {
            submitButton.disabled = false;
        }
    }
);

// Delete truck from SQLite
async function deleteTruck(id) {
    const confirmed = confirm(
        `Are you sure you want to delete truck ${id}?`
    );

    if (!confirmed) return;

    try {
        await apiRequest(`/trucks/${encodeURIComponent(id)}`, {
            method: "DELETE"
        });

        await loadTrucks();

        alert("Truck deleted successfully from SQLite!");

    } catch (error) {
        alert(error.message);

        console.error("Truck deletion error:", error);
    }
}

// Handle Edit and Delete buttons
document.getElementById("truckTableBody").addEventListener(
    "click",
    function(event) {

        const button = event.target.closest(
            "button[data-action]"
        );

        if (!button) return;

        const id = button.dataset.truckId;

        if (button.dataset.action === "edit") {
            editTruck(id);
        }

        if (button.dataset.action === "delete") {
            deleteTruck(id);
        }
    }
);

// Initial load from SQLite
loadTrucks();
