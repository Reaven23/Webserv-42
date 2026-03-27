document.addEventListener("DOMContentLoaded", function () {
    var button = document.getElementById("actionBtn");
    var status = document.getElementById("status");

    if (!button || !status) {
        return;
    }

    button.addEventListener("click", function () {
        var now = new Date();
        status.textContent = "Clicked at " + now.toLocaleTimeString();
    });
});
