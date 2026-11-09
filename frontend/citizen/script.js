
const API_URL = "http://localhost:8080/api";

document.addEventListener("DOMContentLoaded", () => {
    loadCitizenDashboard();
});

async function loadCitizenDashboard() {
    const message = document.getElementById("citizenMessage");

    try {
        const response = await fetch(`${API_URL}/complaints`);

        if (!response.ok) {
            throw new Error("Unable to load complaints");
        }

        const result = await response.json();

        const complaints = Array.isArray(result)
            ? result
            : (result.complaints || []);

        const total = complaints.length;

        const pending = complaints.filter(c =>
            String(c.status).toLowerCase() === "pending"
        ).length;

        const resolved = complaints.filter(c =>
            String(c.status).toLowerCase() === "resolved"
        ).length;

        document.getElementById("totalComplaints").textContent = total;
        document.getElementById("pendingComplaints").textContent = pending;
        document.getElementById("resolvedComplaints").textContent = resolved;

        if (message) {
            message.textContent = "Dashboard updated successfully.";
            message.className = "message success";
        }

    } catch (error) {
        console.error("Dashboard error:", error);

        if (message) {
            message.textContent =
                "Could not connect to the server. Please start the backend.";
            message.className = "message error";
        }
    }
}
