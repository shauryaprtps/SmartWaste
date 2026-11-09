
const API_URL = "http://localhost:8080/api";

const searchForm = document.getElementById("searchComplaintForm");
const searchPhone = document.getElementById("searchPhone");
const statusMessage = document.getElementById("statusMessage");
const complaintResults = document.getElementById("complaintResults");
const refreshButton = document.getElementById("refreshComplaintsBtn");

let lastSearchedPhone = "";

searchForm.addEventListener("submit", function (event) {
    event.preventDefault();

    const phone = searchPhone.value.trim();

    if (!/^[0-9]{10}$/.test(phone)) {
        showMessage("Enter a valid 10-digit mobile number.", "error");
        return;
    }

    lastSearchedPhone = phone;
    loadComplaints(phone);
});

refreshButton.addEventListener("click", function () {
    if (!lastSearchedPhone) {
        showMessage("Search using your mobile number first.", "error");
        return;
    }

    loadComplaints(lastSearchedPhone);
});

async function loadComplaints(phone) {
    complaintResults.replaceChildren();
    complaintResults.textContent = "Loading complaints...";

    try {
        const response = await fetch(`${API_URL}/complaints`);

        if (!response.ok) {
            throw new Error("Unable to fetch complaints.");
        }

        const result = await response.json();

        const complaints = Array.isArray(result)
            ? result
            : result.complaints;

        if (!Array.isArray(complaints)) {
            throw new Error("Unexpected response from server.");
        }

        const myComplaints = complaints.filter(function (complaint) {
            const complaintPhone =
                complaint.citizenPhone ??
                complaint.citizen_phone ??
                complaint.phone ??
                "";

            return String(complaintPhone) === phone;
        });

        complaintResults.replaceChildren();

        if (myComplaints.length === 0) {
            complaintResults.textContent =
                "No complaints found for this mobile number.";

            showMessage("No complaints found.", "error");
            return;
        }

        myComplaints.forEach(function (complaint) {
            const card = document.createElement("div");
            card.className = "content-card";

            const heading = document.createElement("h3");
            heading.textContent =
                `Complaint #${complaint.id ?? complaint.complaintId ?? "N/A"}`;

            const details = document.createElement("p");
            details.textContent =
                `Bin: ${complaint.binId ?? complaint.bin_id ?? "N/A"}`;

            const issue = document.createElement("p");
            issue.textContent =
                `Issue: ${complaint.issueType ?? complaint.issue_type ?? complaint.issue ?? "N/A"}`;

            const description = document.createElement("p");
            description.textContent =
                `Description: ${complaint.description ?? "N/A"}`;

            const status = document.createElement("span");
            const currentStatus = String(
                complaint.status ?? "Pending"
            );

            status.className = "status " + getStatusClass(currentStatus);
            status.textContent = currentStatus;

            const date = document.createElement("p");
            date.textContent =
                `Submitted: ${complaint.createdAt ?? complaint.created_at ?? complaint.createdAt ?? "Date unavailable"}`;

            card.append(heading, details, issue, description, status, date);
            complaintResults.appendChild(card);
        });

        showMessage(
            `${myComplaints.length} complaint(s) found.`,
            "success"
        );

    } catch (error) {
        console.error("Complaint tracking error:", error);

        complaintResults.textContent =
            "Could not load complaints. Please try again.";

        showMessage(error.message, "error");
    }
}

function getStatusClass(status) {
    const value = status.toLowerCase();

    if (value === "resolved" || value === "completed") {
        return "resolved";
    }

    if (value === "in progress" || value === "in-progress") {
        return "in-progress";
    }

    return "pending";
}

function showMessage(message, type) {
    statusMessage.textContent = message;
    statusMessage.className = `message ${type}`;
}
