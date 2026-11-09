// =====================================
// SMARTWASTE CITIZEN PROFILE
// View and edit account information
// =====================================

document.addEventListener("DOMContentLoaded", function () {

    const API_BASE = "http://127.0.0.1:8080";

    let user = {};

    try {
        user = JSON.parse(
            sessionStorage.getItem("smartwasteUser") || "{}"
        );
    } catch (error) {
        console.error("Could not read saved user:", error);
    }

    const token = sessionStorage.getItem("smartwasteToken");

    // =====================================
    // UPDATE PROFILE DISPLAY
    // =====================================

    function updateProfileDisplay() {

        const name = user.name || "Citizen";
        const initial = name.trim().charAt(0).toUpperCase() || "C";

        document.querySelectorAll(
            "#citizenInitial, #profileCircle, .admin-avatar, .citizen-avatar"
        ).forEach(function (element) {
            element.textContent = initial;
        });

        document.querySelectorAll(
            "#citizenDisplayName, .sidebar-bottom strong"
        ).forEach(function (element) {
            element.textContent = name;
        });

        document.querySelectorAll(
            "#citizenDisplayRole, .sidebar-bottom small"
        ).forEach(function (element) {
            element.textContent = "Community Member";
        });
    }

    updateProfileDisplay();

    // =====================================
    // CREATE PROFILE POPUP
    // =====================================

    const modal = document.createElement("div");

    modal.id = "citizenProfileModal";
    modal.className = "citizen-profile-overlay";

    modal.innerHTML = `
        <div class="citizen-profile-modal">

            <div class="citizen-profile-heading">
                <div>
                    <span>SMARTWASTE ACCOUNT</span>
                    <h2>My Profile</h2>
                    <p>View and manage your personal information.</p>
                </div>

                <button type="button"
                        id="closeProfile"
                        class="citizen-profile-close">
                    ×
                </button>
            </div>

            <div class="citizen-profile-user">
                <div class="citizen-profile-large-avatar"
                     id="modalInitial">C</div>

                <div>
                    <h3 id="modalName">Citizen</h3>
                    <span>Verified Citizen Account</span>
                </div>
            </div>

            <form id="profileEditForm">

                <div class="citizen-profile-field">
                    <label>Full Name</label>
                    <input type="text"
                           id="profileName"
                           required
                           maxlength="100"
                           readonly>
                </div>

                <div class="citizen-profile-field">
                    <label>Email Address</label>
                    <input type="email"
                           id="profileEmail"
                           required
                           maxlength="254"
                           readonly>
                </div>

                <div class="citizen-profile-field">
                    <label>Mobile Number</label>
                    <input type="tel"
                           id="profilePhone"
                           required
                           maxlength="10"
                           pattern="[0-9]{10}"
                           readonly>
                </div>

                <div class="citizen-profile-field">
                    <label>Account Type</label>
                    <input type="text"
                           value="Citizen"
                           readonly>
                </div>

                <p id="profileMessage"
                   class="citizen-profile-message"></p>

                <div class="citizen-profile-actions">

                    <button type="button"
                            id="editProfileButton"
                            class="citizen-profile-edit">
                        Edit Profile
                    </button>

                    <button type="button"
                            id="cancelProfileButton"
                            class="citizen-profile-cancel hidden">
                        Cancel
                    </button>

                    <button type="submit"
                            id="saveProfileButton"
                            class="citizen-profile-save hidden">
                        Save Changes
                    </button>

                </div>

            </form>

        </div>
    `;

    document.body.appendChild(modal);

    // =====================================
    // ADD POPUP STYLING
    // =====================================

    const style = document.createElement("style");

    style.textContent = `
        .citizen-profile-overlay {
            display: none;
            position: fixed;
            inset: 0;
            z-index: 9999;
            background: rgba(12, 35, 25, 0.55);
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .citizen-profile-overlay.open {
            display: flex;
        }

        .citizen-profile-modal {
            width: 100%;
            max-width: 450px;
            max-height: 90vh;
            overflow-y: auto;
            background: white;
            border-radius: 15px;
            padding: 27px;
            box-shadow: 0 15px 50px rgba(0,0,0,0.18);
        }

        .citizen-profile-heading {
            display: flex;
            align-items: flex-start;
            justify-content: space-between;
            gap: 15px;
            margin-bottom: 23px;
        }

        .citizen-profile-heading span {
            color: #25815b;
            font-size: 10px;
            font-weight: 750;
            letter-spacing: 1px;
        }

        .citizen-profile-heading h2 {
            color: #193b2c;
            font-size: 23px;
            margin: 7px 0;
        }

        .citizen-profile-heading p {
            color: #819087;
            font-size: 12px;
            line-height: 1.5;
        }

        .citizen-profile-close {
            width: 32px;
            height: 32px;
            border: none;
            border-radius: 8px;
            background: #f0f5f1;
            color: #456153;
            font-size: 23px;
            cursor: pointer;
        }

        .citizen-profile-user {
            display: flex;
            align-items: center;
            gap: 14px;
            padding: 17px;
            background: #eff6f0;
            border-radius: 11px;
            margin-bottom: 23px;
        }

        .citizen-profile-large-avatar {
            width: 48px;
            height: 48px;
            border-radius: 50%;
            display: grid;
            place-items: center;
            background: #176b4b;
            color: white;
            font-size: 20px;
            font-weight: 750;
        }

        .citizen-profile-user h3 {
            color: #214333;
            font-size: 15px;
            margin-bottom: 5px;
        }

        .citizen-profile-user span {
            color: #788b7f;
            font-size: 11px;
        }

        .citizen-profile-field {
            margin-bottom: 16px;
        }

        .citizen-profile-field label {
            display: block;
            color: #405449;
            font-size: 12px;
            font-weight: 700;
            margin-bottom: 7px;
        }

        .citizen-profile-field input {
            width: 100%;
            height: 43px;
            padding: 0 12px;
            border: 1px solid #dce5df;
            border-radius: 8px;
            outline: none;
            color: #30483a;
            font-size: 13px;
            background: #f8faf8;
        }

        .citizen-profile-field input:not([readonly]) {
            background: white;
            border-color: #398866;
        }

        .citizen-profile-actions {
            display: flex;
            justify-content: flex-end;
            gap: 9px;
            margin-top: 20px;
            flex-wrap: wrap;
        }

        .citizen-profile-actions button {
            padding: 11px 17px;
            border: none;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 700;
            cursor: pointer;
        }

        .citizen-profile-edit,
        .citizen-profile-save {
            background: #176b4b;
            color: white;
        }

        .citizen-profile-cancel {
            background: #edf2ee;
            color: #42584b;
        }

        .citizen-profile-message {
            display: none;
            padding: 11px;
            border-radius: 8px;
            font-size: 12px;
            margin-top: 12px;
        }

        .citizen-profile-message.show {
            display: block;
        }

        .citizen-profile-message.success {
            background: #e9f5ec;
            color: #247147;
        }

        .citizen-profile-message.error {
            background: #fff0ee;
            color: #b84c45;
        }

        .citizen-profile-overlay .hidden {
            display: none !important;
        }

        #profileCircle,
        .sidebar-bottom {
            cursor: pointer;
        }

        @media (max-width: 500px) {
            .citizen-profile-modal {
                padding: 21px;
            }
        }
    `;

    document.head.appendChild(style);

    // =====================================
    // POPUP ELEMENTS
    // =====================================

    const nameInput = document.getElementById("profileName");
    const emailInput = document.getElementById("profileEmail");
    const phoneInput = document.getElementById("profilePhone");

    const message = document.getElementById("profileMessage");

    const editButton = document.getElementById("editProfileButton");
    const saveButton = document.getElementById("saveProfileButton");
    const cancelButton = document.getElementById("cancelProfileButton");

    const editForm = document.getElementById("profileEditForm");

    let originalProfile = {};

    // =====================================
    // DISPLAY PROFILE DETAILS
    // =====================================

    function fillProfileFields() {

        nameInput.value = user.name || "";
        emailInput.value = user.email || "";
        phoneInput.value = user.phone || "";

        document.getElementById("modalName").textContent =
            user.name || "Citizen";

        document.getElementById("modalInitial").textContent =
            (user.name || "C").trim().charAt(0).toUpperCase();
    }

    function showProfileMessage(text, type) {

        message.textContent = text;
        message.className =
            "citizen-profile-message show " + type;
    }

    function setEditMode(enabled) {

        nameInput.readOnly = !enabled;
        emailInput.readOnly = !enabled;
        phoneInput.readOnly = !enabled;

        editButton.classList.toggle("hidden", enabled);
        saveButton.classList.toggle("hidden", !enabled);
        cancelButton.classList.toggle("hidden", !enabled);
    }

    // =====================================
    // OPEN PROFILE
    // =====================================

    async function openProfile() {

        modal.classList.add("open");

        setEditMode(false);

        message.className = "citizen-profile-message";

        if (!token) {
            showProfileMessage(
                "Please log in again to access your profile.",
                "error"
            );
            return;
        }

        try {

            const response = await fetch(
                `${API_BASE}/api/auth/profile`,
                {
                    method: "GET",
                    headers: {
                        "Authorization": `Bearer ${token}`
                    }
                }
            );

            const result = await response.json();

            if (!response.ok || !result.success) {
                showProfileMessage(
                    result.message || "Could not load profile.",
                    "error"
                );
                return;
            }

            user = {
                ...user,
                name: result.name,
                email: result.email,
                phone: result.phone,
                role: result.role,
                accountId: result.accountId
            };

            sessionStorage.setItem(
                "smartwasteUser",
                JSON.stringify(user)
            );

            updateProfileDisplay();
            fillProfileFields();

        } catch (error) {

            console.error("Profile loading error:", error);

            showProfileMessage(
                "Unable to connect to the SmartWaste server.",
                "error"
            );
        }
    }

    // =====================================
    // CLOSE PROFILE
    // =====================================

    function closeProfile() {
        modal.classList.remove("open");
        setEditMode(false);
        message.className = "citizen-profile-message";
    }

    document.getElementById("closeProfile")
        .addEventListener("click", closeProfile);

    modal.addEventListener("click", function(event) {
        if (event.target === modal) {
            closeProfile();
        }
    });

    document.addEventListener("keydown", function(event) {
        if (event.key === "Escape") {
            closeProfile();
        }
    });

    // =====================================
    // PROFILE BUTTONS
    // =====================================

    const topProfile = document.getElementById("profileCircle");

    const bottomProfile =
        document.getElementById("profileButton") ||
        document.querySelector(".sidebar-bottom");

    if (topProfile) {
        topProfile.addEventListener("click", openProfile);
    }

    if (bottomProfile) {
        bottomProfile.addEventListener("click", openProfile);
    }

    // =====================================
    // EDIT PROFILE
    // =====================================

    editButton.addEventListener("click", function() {

        originalProfile = {
            name: nameInput.value,
            email: emailInput.value,
            phone: phoneInput.value
        };

        setEditMode(true);

        message.className = "citizen-profile-message";
    });

    // Cancel changes

    cancelButton.addEventListener("click", function() {

        nameInput.value = originalProfile.name;
        emailInput.value = originalProfile.email;
        phoneInput.value = originalProfile.phone;

        setEditMode(false);

        message.className = "citizen-profile-message";
    });

    // =====================================
    // SAVE PROFILE TO SQLITE
    // =====================================

    editForm.addEventListener("submit", async function(event) {

        event.preventDefault();

        const updatedProfile = {
            name: nameInput.value.trim(),
            email: emailInput.value.trim(),
            phone: phoneInput.value.trim()
        };

        if (
            !updatedProfile.name ||
            !updatedProfile.email ||
            !/^[0-9]{10}$/.test(updatedProfile.phone)
        ) {
            showProfileMessage(
                "Please enter a valid name, email and 10-digit phone number.",
                "error"
            );
            return;
        }

        saveButton.disabled = true;
        saveButton.textContent = "Saving...";

        try {

            const response = await fetch(
                `${API_BASE}/api/auth/profile`,
                {
                    method: "PUT",
                    headers: {
                        "Content-Type": "application/json",
                        "Authorization": `Bearer ${token}`
                    },
                    body: JSON.stringify(updatedProfile)
                }
            );

            const result = await response.json();

            if (!response.ok || !result.success) {

                showProfileMessage(
                    result.message || "Could not update profile.",
                    "error"
                );

                return;
            }

            user.name = result.name;
            user.email = result.email;
            user.phone = result.phone;

            sessionStorage.setItem(
                "smartwasteUser",
                JSON.stringify(user)
            );

            updateProfileDisplay();
            fillProfileFields();

            setEditMode(false);

            showProfileMessage(
                "Your profile has been updated successfully!",
                "success"
            );

        } catch (error) {

            console.error("Profile update error:", error);

            showProfileMessage(
                "Unable to save changes. Check your server connection.",
                "error"
            );

        } finally {

            saveButton.disabled = false;
            saveButton.textContent = "Save Changes";
        }
    });

});