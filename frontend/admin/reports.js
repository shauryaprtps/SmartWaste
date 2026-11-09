
const API_URL = "http://localhost:8080/api";

let bins = [];
let trucks = [];
let complaints = [];
let collections = [];

// Get data from the C++ API
async function apiRequest(endpoint) {
    const response = await fetch(API_URL + endpoint);

    if (!response.ok) {
        throw new Error("Could not load " + endpoint);
    }

    return await response.json();
}

// Safely display text inside HTML
function escapeReportHTML(value) {
    return String(value ?? "").replace(/[&<>"']/g, function (char) {
        const entities = {
            "&": "&amp;",
            "<": "&lt;",
            ">": "&gt;",
            '"': "&quot;",
            "'": "&#39;"
        };

        return entities[char];
    });
}

// Update a statistic on the page
function setReportValue(id, value) {
    const element = document.getElementById(id);

    if (element) {
        element.textContent = value;
    }
}

// Format date for display
function formatReportDate(value) {
    if (!value) return "-";

    const date = new Date(String(value).replace(" ", "T"));

    if (isNaN(date.getTime())) {
        return value;
    }

    return date.toLocaleString("en-IN");
}

// Load all report data from SQLite through the API
async function renderReports() {
    try {
        const results = await Promise.all([
            apiRequest("/bins"),
            apiRequest("/trucks"),
            apiRequest("/complaints"),
            apiRequest("/collections")
        ]);

        bins = results[0];
        trucks = results[1];
        complaints = results[2];
        collections = results[3];

        // Calculate total collections and waste
        const totalWaste = collections.reduce(function (total, item) {
            return total + Number(item.wasteCollected || 0);
        }, 0);

        // Count trucks that are not waiting for depot unloading
        const activeTrucks = trucks.filter(function (truck) {
            return truck.status === "Available" ||
                   truck.status === "Assigned";
        }).length;

        // Count resolved complaints
        const resolvedComplaints = complaints.filter(function (item) {
            const status = String(item.status).toLowerCase();

            return status === "resolved" ||
                   status === "closed" ||
                   status === "completed";
        }).length;

        // Update dashboard statistics
        setReportValue("totalCollections", collections.length);
        setReportValue("totalWaste", totalWaste.toFixed(1) + " kg");
        setReportValue("activeTrucks", activeTrucks);
        setReportValue("resolvedComplaints", resolvedComplaints);

        renderBinReport();
        renderFleetReport();
        renderCollectionHistory();

    } catch (error) {
        console.error("Reports loading error:", error);

        setReportValue("totalCollections", "--");
        setReportValue("totalWaste", "--");
        setReportValue("activeTrucks", "--");
        setReportValue("resolvedComplaints", "--");

        const binReport = document.getElementById("binReport");
        const fleetReport = document.getElementById("fleetReport");
        const tableBody = document.getElementById("collectionTableBody");

        if (binReport) {
            binReport.innerHTML =
                '<p class="empty-state">Could not load bin report. Check the API server.</p>';
        }

        if (fleetReport) {
            fleetReport.innerHTML =
                '<p class="empty-state">Could not load fleet report. Check the API server.</p>';
        }

        if (tableBody) {
            tableBody.innerHTML =
                '<tr><td colspan="5">Could not load collection history.</td></tr>';
        }

        setReportValue("historyCount", "Unable to load records");
    }
}

// Generate the bin status report
function renderBinReport() {
    const report = document.getElementById("binReport");

    if (!report) return;

    const critical = bins.filter(bin => Number(bin.fill) > 90).length;

    const attention = bins.filter(bin =>
        Number(bin.fill) >= 75 && Number(bin.fill) <= 90
    ).length;

    const normal = bins.filter(bin =>
        Number(bin.fill) > 0 && Number(bin.fill) < 75
    ).length;

    const empty = bins.filter(bin => Number(bin.fill) === 0).length;

    const categories = [
        { name: "Critical (>90%)", count: critical },
        { name: "Needs Attention (75–90%)", count: attention },
        { name: "Normal (1–74%)", count: normal },
        { name: "Empty (0%)", count: empty }
    ];

    report.innerHTML = categories.map(function (item) {
        return `
            <div class="report-summary-row"
                 style="display:flex;justify-content:space-between;
                        padding:12px 4px;border-bottom:1px solid #edf1ee;">
                <span>${item.name}</span>
                <strong>${item.count}</strong>
            </div>
        `;
    }).join("");

    report.innerHTML += `
        <p class="panel-note">
            Total bins analysed: ${bins.length}
        </p>
    `;
}

// Generate the truck fleet report
function renderFleetReport() {
    const report = document.getElementById("fleetReport");

    if (!report) return;

    const available = trucks.filter(
        truck => truck.status === "Available"
    ).length;

    const assigned = trucks.filter(
        truck => truck.status === "Assigned"
    ).length;

    const needsDepot = trucks.filter(
        truck => truck.status === "NeedsDepot"
    ).length;

    report.innerHTML = `
        <div class="report-summary-row"
             style="display:flex;justify-content:space-between;
                    padding:12px 4px;border-bottom:1px solid #edf1ee;">
            <span>Available Trucks</span>
            <strong>${available}</strong>
        </div>

        <div class="report-summary-row"
             style="display:flex;justify-content:space-between;
                    padding:12px 4px;border-bottom:1px solid #edf1ee;">
            <span>Assigned Trucks</span>
            <strong>${assigned}</strong>
        </div>

        <div class="report-summary-row"
             style="display:flex;justify-content:space-between;
                    padding:12px 4px;border-bottom:1px solid #edf1ee;">
            <span>Needs Unloading</span>
            <strong>${needsDepot}</strong>
        </div>

        <p class="panel-note">
            Total registered trucks: ${trucks.length}
        </p>
    `;
}

// Display collection history in the table
function renderCollectionHistory() {
    const tableBody = document.getElementById("collectionTableBody");

    if (!tableBody) return;

    const searchInput = document.getElementById("reportSearch");

    const searchText = searchInput
        ? searchInput.value.toLowerCase().trim()
        : "";

    const filteredCollections = collections.filter(function (item) {
        const searchableText = [
            item.id,
            item.binId,
            item.truckId,
            item.date,
            item.wasteCollected
        ].join(" ").toLowerCase();

        return searchableText.includes(searchText);
    });

    if (filteredCollections.length === 0) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="5">No collection records found.</td>
            </tr>
        `;

        setReportValue("historyCount", "0 records found");
        return;
    }

    tableBody.innerHTML = filteredCollections.map(function (item) {
        return `
            <tr>
                <td>COL-${escapeReportHTML(item.id)}</td>
                <td>${escapeReportHTML(item.binId)}</td>
                <td>${escapeReportHTML(item.truckId)}</td>
                <td>${Number(item.wasteCollected).toFixed(1)} kg</td>
                <td>${escapeReportHTML(formatReportDate(item.date))}</td>
            </tr>
        `;
    }).join("");

    setReportValue(
        "historyCount",
        filteredCollections.length + " records found"
    );
}

// Load reports when the page opens
document.addEventListener("DOMContentLoaded", function () {
    const searchInput = document.getElementById("reportSearch");

    if (searchInput) {
        searchInput.addEventListener("input", renderCollectionHistory);
    }

    renderReports();
});
