
const API_URL = "http://localhost:8080/api";

const complaintForm = document.getElementById("complaintForm");
const complaintMessage = document.getElementById("complaintMessage");

complaintForm.addEventListener("submit", async function (event) {
    event.preventDefault();

    const citizen = document.getElementById("citizenName").value.trim();
    const phone = document.getElementById("citizenPhone").value.trim();
    const binId = document.getElementById("binId").value.trim().toUpperCase();
    const issue = document.getElementById("issueType").value;
    const description = document.getElementById("description").value.trim();

    if (!/^[0-9]{10}$/.test(phone)) {
        showMessage("Please enter a valid 10-digit mobile number.", "error");
        return;
    }

    if (!citizen || !binId || !issue || !description) {
        showMessage("Please fill in all complaint details.", "error");
        return;
    }

    const complaintData = {
        citizen: citizen,
        phone: phone,
        binId: binId,
        issue: issue,
        description: description
    };

    const submitButton = complaintForm.querySelector("button[type='submit']");
    submitButton.disabled = true;
    submitButton.textContent = "Submitting...";

    try {
        const response = await fetch(`${API_URL}/complaints`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(complaintData)
        });

        const result = await response.json();

        if (!response.ok || !result.success) {
            throw new Error(result.message || "Could not submit complaint.");
        }

        showMessage(
            result.message || "Complaint submitted successfully!",
            "success"
        );

        complaintForm.reset();

    } catch (error) {
        console.error("Complaint submission error:", error);

        showMessage(
            error.message || "Unable to connect to the server. Please try again.",
            "error"
        );

    } finally {
        submitButton.disabled = false;
        submitButton.textContent = "Submit Complaint";
    }
});

function showMessage(message, type) {
    complaintMessage.textContent = message;
    complaintMessage.className = `message ${type}`;
}
