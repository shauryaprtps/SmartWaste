
const API_URL = "http://localhost:8080/api";

let drivers = [];
let trucks = [];
let editingDriverId = null;
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

// Send requests to the C++ API
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

// Determine driver status
function getDriverStatus(driver) {
    return driver.truckId ? "Assigned" : "Unassigned";
}

// Load drivers and trucks from SQLite through the API
async function loadDrivers() {
    isLoading = true;
    loadError = "";

    renderDrivers();

    try {
        const [driverData, truckData] = await Promise.all([
            apiRequest("/drivers", {
                method: "GET",
                cache: "no-store"
            }),

            apiRequest("/trucks", {
                method: "GET",
                cache: "no-store"
            })
        ]);

        if (!Array.isArray(driverData) ||
            !Array.isArray(truckData)) {
            throw new Error("Invalid data received from server.");
        }

        drivers = driverData.map(driver => ({
            id: String(driver.id),
            name: String(driver.name),
            phone: String(driver.phone || ""),
            truckId: String(driver.truckId || "")
        }));

        trucks = truckData.map(truck => ({
            id: String(truck.id),
            status: String(truck.status)
        }));

        isLoading = false;

        renderDrivers();

        return true;

    } catch (error) {
        isLoading = false;
        loadError = error.message;

        renderDrivers();

        console.error("Driver loading error:", error);

        return false;
    }
}

// Refresh button
async function refreshDrivers() {
    await loadDrivers();
}

// Update driver statistics
function updateDriverStatistics() {
    document.getElementById("totalDrivers").textContent =
        drivers.length;

    document.getElementById("assignedDrivers").textContent =
        drivers.filter(driver => driver.truckId !== "").length;

    document.getElementById("unassignedDrivers").textContent =
        drivers.filter(driver => driver.truckId === "").length;
}

// Populate truck dropdown using actual database trucks
function populateTruckDropdown(selectedTruck = "", currentDriverId = null) {
    const dropdown = document.getElementById("driverTruck");

    dropdown.innerHTML = "";

    // Option for an unassigned driver
    const emptyOption = document.createElement("option");

    emptyOption.value = "";
    emptyOption.textContent = "No truck assigned";

    dropdown.appendChild(emptyOption);

    trucks.forEach(truck => {

        // Prevent assigning a truck already linked to another driver
        const usedByAnotherDriver = drivers.some(driver =>
            driver.truckId === truck.id &&
            driver.id !== currentDriverId
        );

        if (usedByAnotherDriver) {
            return;
        }

        const option = document.createElement("option");

        option.value = truck.id;
        option.textContent = `${truck.id} (${truck.status})`;

        if (truck.id === selectedTruck) {
            option.selected = true;
        }

        dropdown.appendChild(option);
    });

    dropdown.value = selectedTruck;
}

// Display drivers
function renderDrivers() {
    const tableBody = document.getElementById("driverTableBody");

    const searchInput = document.getElementById("driverSearch");
    const filterInput = document.getElementById("driverFilter");

    if (!tableBody || !searchInput || !filterInput) return;

    const search = searchInput.value.trim().toLowerCase();
    const filter = filterInput.value;

    if (isLoading) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="6" style="text-align:center;padding:25px;">
                    Loading drivers from SQLite...
                </td>
            </tr>
        `;

        document.getElementById("driverCount").textContent =
            "Connecting to SmartWaste database...";

        return;
    }

    if (loadError) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="6" style="text-align:center;padding:25px;">
                    Unable to load drivers. Check the C++ API server.
                </td>
            </tr>
        `;

        document.getElementById("driverCount").textContent =
            loadError;

        ["totalDrivers", "assignedDrivers",
         "unassignedDrivers"].forEach(id => {
            document.getElementById(id).textContent = "--";
        });

        return;
    }

    const filteredDrivers = drivers.filter(driver => {
        const status = getDriverStatus(driver);

        const matchesSearch =
            driver.id.toLowerCase().includes(search) ||
            driver.name.toLowerCase().includes(search);

        const matchesFilter =
            filter === "All" || status === filter;

        return matchesSearch && matchesFilter;
    });

    tableBody.innerHTML = filteredDrivers.map(driver => {
        const status = getDriverStatus(driver);

        const statusClass =
            status === "Assigned" ? "assigned" : "unassigned";

        return `
            <tr>
                <td><strong>${escapeHTML(driver.id)}</strong></td>

                <td>${escapeHTML(driver.name)}</td>

                <td>${escapeHTML(driver.phone)}</td>

                <td>${escapeHTML(driver.truckId || "Not assigned")}</td>

                <td>
                    <span class="driver-status ${statusClass}">
                        ${status}
                    </span>
                </td>

                <td>
                    <button class="table-action edit-action"
                        data-action="edit"
                        data-driver-id="${escapeHTML(driver.id)}">
                        Edit
                    </button>

                    <button class="table-action delete-action"
                        data-action="delete"
                        data-driver-id="${escapeHTML(driver.id)}">
                        Delete
                    </button>
                </td>
            </tr>
        `;
    }).join("");

    if (filteredDrivers.length === 0) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="6" style="text-align:center;padding:25px;">
                    No drivers found.
                </td>
            </tr>
        `;
    }

    document.getElementById("driverCount").textContent =
        `Showing ${filteredDrivers.length} of ${drivers.length} drivers. Live data connected to SQLite database.`;

    updateDriverStatistics();
}

// Open the add driver modal
function openDriverModal() {
    editingDriverId = null;

    const form = document.getElementById("driverForm");

    form.reset();

    document.getElementById("driverModalTitle").textContent =
        "Add New Driver";

    document.getElementById("driverId").disabled = false;

    document.getElementById("driverFormError").textContent = "";

    populateTruckDropdown();

    document.getElementById("driverModal").classList.add("show");
}

// Close modal
function closeDriverModal() {
    document.getElementById("driverModal").classList.remove("show");

    document.getElementById("driverForm").reset();

    document.getElementById("driverId").disabled = false;

    document.getElementById("driverFormError").textContent = "";

    editingDriverId = null;
}

// Edit an existing driver
function editDriver(id) {
    const driver = drivers.find(item => item.id === id);

    if (!driver) return;

    editingDriverId = id;

    document.getElementById("driverModalTitle").textContent =
        "Edit Driver";

    document.getElementById("driverId").value = driver.id;

    document.getElementById("driverId").disabled = true;

    document.getElementById("driverName").value =
        driver.name;

    document.getElementById("driverPhone").value =
        driver.phone;

    document.getElementById("driverFormError").textContent = "";

    populateTruckDropdown(driver.truckId, driver.id);

    document.getElementById("driverModal").classList.add("show");
}

// Add or update a driver in SQLite
document.getElementById("driverForm").addEventListener(
    "submit",
    async function(event) {

        event.preventDefault();

        const id = document.getElementById("driverId")
            .value.trim().toUpperCase();

        const name = document.getElementById("driverName")
            .value.trim();

        const phone = document.getElementById("driverPhone")
            .value.trim();

        const truckId = document.getElementById("driverTruck")
            .value;

        const error = document.getElementById("driverFormError");

        if (!/^D[0-9]+$/.test(id) || !name ||
            !/^[0-9]{10}$/.test(phone)) {

            error.textContent =
                "Enter a valid Driver ID (example: D102), name and 10-digit phone number.";

            return;
        }

        // Confirm selected truck exists in database records
        const selectedTruckExists = !truckId ||
            trucks.some(truck => truck.id === truckId);

        if (!selectedTruckExists) {
            error.textContent = "The selected truck does not exist.";

            return;
        }

        // Check whether another driver already has this truck
        const truckAlreadyAssigned = drivers.some(driver =>
            driver.truckId === truckId &&
            truckId !== "" &&
            driver.id !== editingDriverId
        );

        if (truckAlreadyAssigned) {
            error.textContent =
                "This truck is already assigned to another driver.";

            return;
        }

        const payload = {
            name,
            phone,
            truckId
        };

        let endpoint = "/drivers";
        let method = "POST";

        if (editingDriverId !== null) {
            endpoint = `/drivers/${encodeURIComponent(editingDriverId)}`;
            method = "PUT";
        } else {
            payload.id = id;
        }

        const submitButton = this.querySelector(
            'button[type="submit"]'
        );

        submitButton.disabled = true;

        error.textContent = "Saving driver to SQLite...";

        try {
            const wasEditing = editingDriverId !== null;

            await apiRequest(endpoint, {
                method,
                body: JSON.stringify(payload)
            });

            closeDriverModal();

            const loaded = await loadDrivers();

            if (!loaded) {
                alert(
                    "Driver saved, but refreshing the list failed. Use Refresh."
                );
                return;
            }

            alert(
                wasEditing
                    ? "Driver updated successfully in SQLite!"
                    : "Driver added successfully to SQLite!"
            );

        } catch (error) {
            document.getElementById("driverFormError").textContent =
                error.message;

            console.error("Driver save error:", error);

        } finally {
            submitButton.disabled = false;
        }
    }
);

// Delete driver from SQLite
async function deleteDriver(id) {
    const confirmed = confirm(
        `Are you sure you want to delete driver ${id}?`
    );

    if (!confirmed) return;

    try {
        await apiRequest(`/drivers/${encodeURIComponent(id)}`, {
            method: "DELETE"
        });

        await loadDrivers();

        alert("Driver deleted successfully from SQLite!");

    } catch (error) {
        alert(error.message);

        console.error("Driver deletion error:", error);
    }
}

// Handle Edit and Delete button clicks
document.getElementById("driverTableBody").addEventListener(
    "click",
    function(event) {

        const button = event.target.closest(
            "button[data-action]"
        );

        if (!button) return;

        const id = button.dataset.driverId;

        if (button.dataset.action === "edit") {
            editDriver(id);
        }

        if (button.dataset.action === "delete") {
            deleteDriver(id);
        }
    }
);

// Close modal when clicking outside
document.getElementById("driverModal").addEventListener(
    "click",
    function(event) {

        if (event.target === this) {
            closeDriverModal();
        }
    }
);

// Initial load from SQLite
loadDrivers();
