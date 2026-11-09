const API_URL = "http://localhost:8080/api";

let bins = [];
let isLoading = false;
let loadError = "";

// Determine bin priority
function getPriority(fill) {
    fill = Number(fill);

    if (fill > 90) return "Critical";
    if (fill >= 75) return "High";
    if (fill > 0) return "Normal";

    return "Empty";
}

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

// Load bins from SQLite
async function loadBins() {
    isLoading = true;
    loadError = "";

    renderBins();

    try {
        const result = await apiRequest("/bins", {
            method: "GET",
            cache: "no-store"
        });

        if (!Array.isArray(result)) {
            throw new Error("Invalid bin data received from server.");
        }

        bins = result.map(bin => ({
            id: String(bin.id),
            location: String(bin.location),
            locationNode: Number(bin.locationNode),
            capacity: Number(bin.capacity),
            fill: Number(bin.fill),
            type: String(bin.type)
        }));

        isLoading = false;
        renderBins();

        return true;

    } catch (error) {
        isLoading = false;
        loadError = error.message;

        renderBins();

        console.error("Bin loading error:", error);

        return false;
    }
}

// Refresh button
async function refreshBins() {
    await loadBins();
}

// Display bins in the table
function renderBins() {
    const searchInput = document.getElementById("binSearch");
    const filterInput = document.getElementById("fillFilter");
    const table = document.getElementById("binTableBody");

    if (!searchInput || !filterInput || !table) return;

    const search = searchInput.value.toLowerCase().trim();
    const filter = filterInput.value;

    if (isLoading) {
        table.innerHTML = `
            <tr>
                <td colspan="7" style="text-align:center;padding:25px;">
                    Loading bins from SQLite...
                </td>
            </tr>
        `;

        document.getElementById("binMessage").textContent =
            "Connecting to the SmartWaste database...";

        return;
    }

    if (loadError) {
        table.innerHTML = `
            <tr>
                <td colspan="7" style="text-align:center;padding:25px;">
                    Could not load bins. Check whether the C++ API server is running.
                </td>
            </tr>
        `;

        document.getElementById("binMessage").textContent = loadError;
        return;
    }

    const filteredBins = bins.filter(bin => {
        const matchesSearch =
            bin.id.toLowerCase().includes(search) ||
            bin.location.toLowerCase().includes(search);

        const matchesFilter =
            filter === "All" ||
            getPriority(bin.fill) === filter;

        return matchesSearch && matchesFilter;
    });

    table.innerHTML = filteredBins.map(bin => {
        const waste = (
            bin.capacity * bin.fill / 100
        ).toFixed(0);

        const priority = getPriority(bin.fill);

        return `
            <tr>
                <td><strong>${escapeHTML(bin.id)}</strong></td>
                <td>${escapeHTML(bin.location)}</td>
                <td>${bin.fill}%</td>
                <td>${escapeHTML(bin.type)}</td>
                <td>${waste} kg</td>
                <td>
                    <span class="bin-status ${priority.toLowerCase()}">
                        ${priority}
                    </span>
                </td>
                <td>
                    <button class="bin-action bin-edit"
                        data-action="edit"
                        data-bin-id="${escapeHTML(bin.id)}">
                        Edit
                    </button>

                    <button class="bin-action bin-delete"
                        data-action="delete"
                        data-bin-id="${escapeHTML(bin.id)}">
                        Delete
                    </button>
                </td>
            </tr>
        `;
    }).join("");

    if (filteredBins.length === 0) {
        table.innerHTML = `
            <tr>
                <td colspan="7" style="text-align:center;padding:25px;">
                    No bins found.
                </td>
            </tr>
        `;
    }

    // Update statistics from database records
    document.getElementById("binTotal").textContent =
        bins.length;

    document.getElementById("criticalTotal").textContent =
        bins.filter(bin => bin.fill > 90).length;

    document.getElementById("attentionTotal").textContent =
        bins.filter(bin =>
            bin.fill >= 75 && bin.fill <= 90
        ).length;

    document.getElementById("emptyTotal").textContent =
        bins.filter(bin => bin.fill === 0).length;

    document.getElementById("binMessage").textContent =
        `Showing ${filteredBins.length} of ${bins.length} bins. Live data connected to SQLite database.`;
}

// Open modal for adding a bin
function openBinModal() {
    const form = document.getElementById("binForm");

    form.reset();
    form.dataset.editingId = "";

    document.getElementById("binId").disabled = false;
    document.getElementById("binLocationNode").value = "0";
    document.getElementById("modalTitle").textContent = "Add New Bin";
    document.getElementById("binFormError").textContent = "";

    document.getElementById("binModal").classList.add("show");
}

// Close modal
function closeBinModal() {
    const form = document.getElementById("binForm");

    document.getElementById("binModal").classList.remove("show");

    form.reset();
    form.dataset.editingId = "";

    document.getElementById("binId").disabled = false;
    document.getElementById("binFormError").textContent = "";
}

// Add or edit a bin in SQLite
document.getElementById("binForm").addEventListener(
    "submit",
    async function(event) {
        event.preventDefault();

        const id = document.getElementById("binId")
            .value.trim().toUpperCase();

        const location = document.getElementById("binLocation")
            .value.trim();

        const locationNode = Number(
            document.getElementById("binLocationNode").value
        );

        const capacity = Number(
            document.getElementById("binCapacity").value
        );

        const fill = Number(
            document.getElementById("binFill").value
        );

        const type = document.getElementById("binType").value;

        const editingId = this.dataset.editingId;
        const error = document.getElementById("binFormError");

        if (
            !id ||
            !location ||
            !Number.isInteger(locationNode) ||
            locationNode < 0 ||
            locationNode > 3 ||
            !Number.isFinite(capacity) ||
            !Number.isFinite(fill) ||
            capacity <= 0 ||
            fill < 0 ||
            fill > 100
        ) {
            error.textContent =
                "Please enter valid bin details and road network node.";
            return;
        }

        const payload = {
            location,
            locationNode,
            capacity,
            fill,
            type
        };

        let endpoint = "/bins";
        let method = "POST";

        if (editingId) {
            endpoint = `/bins/${encodeURIComponent(editingId)}`;
            method = "PUT";
        } else {
            payload.id = id;
        }

        const submitButton = this.querySelector(
            'button[type="submit"]'
        );

        submitButton.disabled = true;
        error.textContent = "Saving to SQLite...";

        try {
            await apiRequest(endpoint, {
                method,
                body: JSON.stringify(payload)
            });

            const loaded = await loadBins();

            if (!loaded) {
                throw new Error(
                    "Saved, but refreshing the bin list failed."
                );
            }

            closeBinModal();

            alert(
                editingId
                    ? "Bin updated successfully in SQLite!"
                    : "New bin added successfully to SQLite!"
            );

        } catch (error) {
            document.getElementById("binFormError").textContent =
                error.message;

            console.error("Bin save error:", error);

        } finally {
            submitButton.disabled = false;
        }
    }
);

// Edit existing bin
function editBin(id) {
    const bin = bins.find(item => item.id === id);

    if (!bin) return;

    document.getElementById("modalTitle").textContent = "Edit Bin";
    document.getElementById("binId").value = bin.id;
    document.getElementById("binId").disabled = true;
    document.getElementById("binLocation").value = bin.location;

    document.getElementById("binLocationNode").value =
        String(bin.locationNode);

    document.getElementById("binCapacity").value = bin.capacity;
    document.getElementById("binFill").value = bin.fill;
    document.getElementById("binType").value = bin.type;

    document.getElementById("binForm").dataset.editingId = bin.id;
    document.getElementById("binFormError").textContent = "";

    document.getElementById("binModal").classList.add("show");
}

// Delete bin from SQLite
async function deleteBin(id) {
    const confirmDelete = confirm(
        `Are you sure you want to delete bin ${id}?`
    );

    if (!confirmDelete) return;

    try {
        await apiRequest(`/bins/${encodeURIComponent(id)}`, {
            method: "DELETE"
        });

        await loadBins();

        alert("Bin deleted successfully from SQLite!");

    } catch (error) {
        alert(error.message);
        console.error("Bin deletion error:", error);
    }
}

// Handle Edit and Delete clicks
document.getElementById("binTableBody").addEventListener(
    "click",
    function(event) {
        const button = event.target.closest(
            "button[data-action]"
        );

        if (!button) return;

        const id = button.dataset.binId;

        if (button.dataset.action === "edit") {
            editBin(id);
        }

        if (button.dataset.action === "delete") {
            deleteBin(id);
        }
    }
);

// Close modal when clicking outside
document.getElementById("binModal").addEventListener(
    "click",
    function(event) {
        if (event.target === this) {
            closeBinModal();
        }
    }
);

// Load database records when page opens
loadBins();
