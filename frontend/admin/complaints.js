
const API_URL = "http://localhost:8080/api";

let complaints = [];
let selectedComplaintId = null;
let isLoading = false;
let loadError = "";

// Safely display text
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

// Load complaints from SQLite
async function loadComplaints() {
    isLoading = true;
    loadError = "";

    renderComplaints();

    try {
        const result = await apiRequest("/complaints", {
            method: "GET",
            cache: "no-store"
        });

        if (!Array.isArray(result)) {
            throw new Error("Invalid complaint data received.");
        }

        complaints = result.map(complaint => ({
            id: String(complaint.id),
            binId: String(complaint.binId || ""),
            citizen: String(complaint.citizen || ""),
            phone: String(complaint.phone || ""),
            issue: String(complaint.issue || "Other"),
            description: String(complaint.description || ""),
            date: String(complaint.date || ""),
            status: String(complaint.status || "Pending")
        }));

        isLoading = false;
        renderComplaints();

        return true;

    } catch (error) {
        isLoading = false;
        loadError = error.message;

        renderComplaints();

        console.error("Complaint loading error:", error);

        return false;
    }
}

// Refresh button
async function refreshComplaints() {
    await loadComplaints();
}

// Update complaint statistics
function updateComplaintStatistics() {
    document.getElementById("totalComplaints").textContent =
        complaints.length;

    document.getElementById("pendingComplaints").textContent =
        complaints.filter(c => c.status === "Pending").length;

    document.getElementById("progressComplaints").textContent =
        complaints.filter(c => c.status === "In Progress").length;

    document.getElementById("resolvedComplaints").textContent =
        complaints.filter(c => c.status === "Resolved").length;
}

// Display complaints
function renderComplaints() {
    const tableBody =
        document.getElementById("complaintTableBody");

    const searchInput =
        document.getElementById("complaintSearch");

    const filterInput =
        document.getElementById("complaintFilter");

    if (!tableBody || !searchInput || !filterInput) return;

    const search = searchInput.value.trim().toLowerCase();
    const filter = filterInput.value;

    if (isLoading) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="7" style="text-align:center;padding:25px;">
                    Loading complaints from SQLite...
                </td>
            </tr>
        `;

        document.getElementById("complaintCount").textContent =
            "Connecting to SmartWaste database...";

        return;
    }

    if (loadError) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="7" style="text-align:center;padding:25px;">
                    Unable to load complaints. Check the C++ API server.
                </td>
            </tr>
        `;

        document.getElementById("complaintCount").textContent =
            loadError;

        [
            "totalComplaints",
            "pendingComplaints",
            "progressComplaints",
            "resolvedComplaints"
        ].forEach(id => {
            document.getElementById(id).textContent = "--";
        });

        return;
    }

    const filteredComplaints = complaints.filter(complaint => {
        const matchesSearch =
            complaint.id.toLowerCase().includes(search) ||
            complaint.binId.toLowerCase().includes(search) ||
            complaint.citizen.toLowerCase().includes(search);

        const matchesFilter =
            filter === "All" || complaint.status === filter;

        return matchesSearch && matchesFilter;
    });

    tableBody.innerHTML = filteredComplaints.map(complaint => {
        let statusClass = "pending";

        if (complaint.status === "In Progress") {
            statusClass = "in-progress";
        }

        if (complaint.status === "Resolved") {
            statusClass = "resolved";
        }

        return `
            <tr>
                <td><strong>${escapeHTML(complaint.id)}</strong></td>

                <td>${escapeHTML(complaint.binId || "N/A")}</td>

                <td>${escapeHTML(complaint.citizen)}</td>

                <td>${escapeHTML(complaint.issue)}</td>

                <td>${escapeHTML(complaint.date)}</td>

                <td>
                    <span class="complaint-status ${statusClass}">
                        ${escapeHTML(complaint.status)}
                    </span>
                </td>

                <td>
                    <button class="table-action edit-action"
                        data-action="view"
                        data-complaint-id="${escapeHTML(complaint.id)}">
                        View
                    </button>

                    <button class="table-action delete-action"
                        data-action="delete"
                        data-complaint-id="${escapeHTML(complaint.id)}">
                        Delete
                    </button>
                </td>
            </tr>
        `;
    }).join("");

    if (filteredComplaints.length === 0) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="7" style="text-align:center;padding:25px;">
                    No complaints found.
                </td>
            </tr>
        `;
    }

    document.getElementById("complaintCount").textContent =
        `Showing ${filteredComplaints.length} of ${complaints.length} complaints. Live data connected to SQLite database.`;

    updateComplaintStatistics();
}

// Open complaint details
function viewComplaint(id) {
    const complaint = complaints.find(c => c.id === id);

    if (!complaint) return;

    selectedComplaintId = id;

    document.getElementById("detailId").textContent =
        complaint.id;

    document.getElementById("detailBin").textContent =
        complaint.binId || "N/A";

    document.getElementById("detailCitizen").textContent =
        complaint.citizen;

    document.getElementById("detailPhone").textContent =
        complaint.phone;

    document.getElementById("detailDate").textContent =
        complaint.date;

    document.getElementById("detailDescription").textContent =
        complaint.description;

    document.getElementById("complaintStatus").value =
        complaint.status;

    document.getElementById("complaintError").textContent = "";

    document.getElementById("complaintModal").classList.add("show");
}

// Close complaint modal
function closeComplaintModal() {
    document.getElementById("complaintModal").classList.remove("show");

    document.getElementById("complaintError").textContent = "";

    selectedComplaintId = null;
}

// Update complaint status in SQLite
async function updateComplaintStatus() {
    if (!selectedComplaintId) return;

    const newStatus =
        document.getElementById("complaintStatus").value;

    const error =
        document.getElementById("complaintError");

    try {
        await apiRequest(
            `/complaints/${encodeURIComponent(selectedComplaintId)}`,
            {
                method: "PUT",
                body: JSON.stringify({
                    status: newStatus
                })
            }
        );

        const loaded = await loadComplaints();

        if (!loaded) {
            throw new Error(
                "Status updated, but refreshing the complaint list failed."
            );
        }

        closeComplaintModal();

        alert("Complaint status updated successfully in SQLite!");

    } catch (error) {
        document.getElementById("complaintError").textContent =
            error.message;

        console.error("Complaint status update error:", error);
    }
}

// Delete complaint from SQLite
async function deleteComplaint(id) {
    const confirmed = confirm(
        `Are you sure you want to delete complaint ${id}?`
    );

    if (!confirmed) return;

    try {
        await apiRequest(
            `/complaints/${encodeURIComponent(id)}`,
            {
                method: "DELETE"
            }
        );

        await loadComplaints();

        alert("Complaint deleted successfully from SQLite!");

    } catch (error) {
        alert(error.message);

        console.error("Complaint deletion error:", error);
    }
}

// Handle View and Delete button clicks
document.getElementById("complaintTableBody").addEventListener(
    "click",
    function(event) {
        const button = event.target.closest(
            "button[data-action]"
        );

        if (!button) return;

        const id = button.dataset.complaintId;

        if (button.dataset.action === "view") {
            viewComplaint(id);
        }

        if (button.dataset.action === "delete") {
            deleteComplaint(id);
        }
    }
);

// Close modal when clicking outside
document.getElementById("complaintModal").addEventListener(
    "click",
    function(event) {
        if (event.target === this) {
            closeComplaintModal();
        }
    }
);

// Initial loading from SQLite
loadComplaints();
