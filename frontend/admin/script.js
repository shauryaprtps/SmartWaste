
const API_URL = "http://localhost:8080/api";

// Fetch data from the C++ API
async function fetchDashboardData(endpoint) {
    const response = await fetch(`${API_URL}${endpoint}`, {
        method: "GET",
        cache: "no-store"
    });

    if (!response.ok) {
        throw new Error(`Could not load ${endpoint}`);
    }

    const data = await response.json();

    if (!Array.isArray(data)) {
        throw new Error(`Invalid data received from ${endpoint}`);
    }

    return data;
}

// Calculate truck status
function getDashboardTruckStatus(truck) {
    const capacity = Number(truck.capacity) || 0;
    const load = Number(truck.load) || 0;

    if (capacity > 0 && load >= capacity * 0.90) {
        return "NeedsDepot";
    }

    return truck.status || "Available";
}

// Load all Dashboard information from SQLite
async function renderDashboard() {
    try {
        const [bins, trucks, complaints] = await Promise.all([
            fetchDashboardData("/bins"),
            fetchDashboardData("/trucks"),
            fetchDashboardData("/complaints")
        ]);

        const availableTrucks = trucks.filter(
            truck => getDashboardTruckStatus(truck) === "Available"
        ).length;

        const pendingComplaints = complaints.filter(
            complaint => complaint.status === "Pending"
        ).length;

        const criticalBins = bins.filter(
            bin => Number(bin.fill) > 90
        );

        // Count trucks currently marked as Assigned.
        const activeAssignments = trucks.filter(
            truck => getDashboardTruckStatus(truck) === "Assigned"
        ).length;

        // Update Dashboard statistics
        setText("totalBins", bins.length);
        setText("totalTrucks", trucks.length);
        setText("availableTrucks", availableTrucks);
        setText("pendingComplaints", pendingComplaints);
        setText("criticalBins", criticalBins.length);
        setText("activeAssignments", activeAssignments);

        // Calculate the percentage of bins not in critical condition.
        // This is an operational indicator, not actual collection completion.
        const nonCriticalCount = bins.filter(
            bin => Number(bin.fill) <= 90
        ).length;

        const systemStatus = bins.length
            ? Math.round((nonCriticalCount / bins.length) * 100)
            : 0;

        setText("collectionProgress", `${systemStatus}%`);

        const progressFill = document.getElementById("progressFill");

        if (progressFill) {
            progressFill.style.width = `${systemStatus}%`;
        }

        // Display recent activity from database records
        renderRecentActivity(
            bins,
            trucks,
            complaints,
            criticalBins
        );

        console.log("Dashboard data loaded successfully from SQLite.");

    } catch (error) {
        console.error("Dashboard loading error:", error);

        // Show error instead of displaying misleading statistics
        [
            "totalBins",
            "totalTrucks",
            "availableTrucks",
            "pendingComplaints",
            "criticalBins",
            "activeAssignments",
            "collectionProgress"
        ].forEach(id => setText(id, "--"));

        const container = document.getElementById("recentActivity");

        if (container) {
            container.innerHTML = `
                <div class="activity-empty-state">
                    <strong>Unable to load Dashboard data.</strong>
                    <small>Please make sure the C++ API server is running.</small>
                </div>
            `;
        }
    }
}

// Update an HTML element
function setText(id, value) {
    const element = document.getElementById(id);

    if (element) {
        element.textContent = value;
    }
}

// Display recent system activity
function renderRecentActivity(bins, trucks, complaints, criticalBins) {
    const container = document.getElementById("recentActivity");

    if (!container) return;

    const activities = [];

    // Critical bins
    criticalBins.forEach(bin => {
        activities.push({
            type: "critical",
            label: "URGENT BIN",
            title: `Bin ${bin.id} requires urgent collection`,
            detail: `${Number(bin.fill)}% full · ${bin.location || "Location not set"}`
        });
    });

    // Trucks requiring unloading
    trucks.filter(truck =>
        getDashboardTruckStatus(truck) === "NeedsDepot"
    ).forEach(truck => {
        activities.push({
            type: "truck",
            label: "DEPOT REQUIRED",
            title: `Truck ${truck.id} needs unloading`,
            detail: `${truck.load} kg of ${truck.capacity} kg capacity`
        });
    });

    // Pending complaints
    complaints.filter(complaint =>
        complaint.status === "Pending"
    ).forEach(complaint => {
        activities.push({
            type: "complaint",
            label: "PENDING COMPLAINT",
            title: `Complaint ${complaint.id} is pending`,
            detail: `${complaint.issue || "Citizen complaint"} · Bin ${complaint.binId || "N/A"}`
        });
    });

    // No urgent activities
    if (activities.length === 0) {
        container.innerHTML = `
            <div class="activity-empty-state">
                <div class="activity-empty-icon">✓</div>

                <div>
                    <strong>You're all caught up!</strong>
                    <small>No urgent bins, trucks or pending complaints right now.</small>
                </div>
            </div>
        `;

        return;
    }

    const icons = {
        critical: `
            <svg viewBox="0 0 24 24" aria-hidden="true">
                <path d="M12 3 22 21H2L12 3Z"/>
                <path d="M12 9v5M12 17h.01"/>
            </svg>
        `,

        truck: `
            <svg viewBox="0 0 24 24" aria-hidden="true">
                <path d="M3 6h11v12H3zM14 10h4l3 3v5h-7z"/>
                <circle cx="7.5" cy="18" r="1.8"/>
                <circle cx="17.5" cy="18" r="1.8"/>
            </svg>
        `,

        complaint: `
            <svg viewBox="0 0 24 24" aria-hidden="true">
                <path d="M6 3h9l4 4v14H6z"/>
                <path d="M14 3v5h5M9 12h6M9 16h6"/>
            </svg>
        `
    };

    container.innerHTML = activities.slice(0, 6).map(item => `
        <div class="dashboard-activity-item activity-${item.type}">

            <div class="dashboard-activity-icon">
                ${icons[item.type]}
            </div>

            <div class="dashboard-activity-content">

                <span class="dashboard-activity-type">
                    ${escapeDashboardHTML(item.label)}
                </span>

                <strong>
                    ${escapeDashboardHTML(item.title)}
                </strong>

                <small>
                    ${escapeDashboardHTML(item.detail)}
                </small>

            </div>

        </div>
    `).join("");
}

// Prevent HTML injection
function escapeDashboardHTML(value) {
    return String(value).replace(/[&<>"']/g, character => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[character]);
}

// Load data when Dashboard opens
renderDashboard();

// Refresh data when returning to Dashboard
window.addEventListener("focus", renderDashboard);
