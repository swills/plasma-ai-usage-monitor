.pragma library

function labelValue(value) {
    return (value || "").toString().replace(/\\/g, "\\\\").replace(/"/g, "\\\"");
}

function appendToolQuotaMetrics(lines, toolKey, windows) {
    var actualSources = ["billing_api", "usage_api", "actual_api",
                         "metrics_api", "response_headers", "browser_sync",
                         "antigravity_local", "local_daemon_actual"];
    for (var i = 0; i < windows.length; i++) {
        var window = windows[i] || {};
        var source = String(window.source || "");
        var remaining = Number(window.percentRemaining);
        if (actualSources.indexOf(source) < 0 || !Number.isFinite(remaining))
            continue;
        var labels = "tool=\"" + labelValue(toolKey)
            + "\",kind=\"" + labelValue(window.kind)
            + "\",source=\"" + labelValue(source)
            + "\",quality=\"" + labelValue(window.precision || window.sourceClass) + "\"";
        lines.push("ai_usage_tool_quota_percent_remaining{" + labels + "} " + remaining);
        var resetTimestamp = Date.parse(window.resetAt) / 1000;
        if (Number.isFinite(resetTimestamp)) {
            lines.push("ai_usage_tool_quota_reset_timestamp_seconds{" + labels + "} "
                       + resetTimestamp);
        }
    }
}
