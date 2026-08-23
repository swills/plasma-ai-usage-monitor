import QtQuick
import QtTest
import "../../../../package/contents/ui/PrometheusMetrics.js" as PrometheusMetrics

TestCase {
    name: "PrometheusMetrics"

    function test_actualQuotaWindow() {
        var lines = [];
        PrometheusMetrics.appendToolQuotaMetrics(lines, "codex_cli", [{
            kind: "rolling_5h",
            percentRemaining: 96,
            resetAt: "2026-08-23T12:00:00Z",
            source: "browser_sync",
            precision: "browser_sync_actual"
        }]);

        compare(lines.length, 2);
        compare(lines[0], "ai_usage_tool_quota_percent_remaining{tool=\"codex_cli\",kind=\"rolling_5h\",source=\"browser_sync\",quality=\"browser_sync_actual\"} 96");
        compare(lines[1], "ai_usage_tool_quota_reset_timestamp_seconds{tool=\"codex_cli\",kind=\"rolling_5h\",source=\"browser_sync\",quality=\"browser_sync_actual\"} 1787486400");
    }

    function test_nonActualAndInvalidQuotaWindows() {
        var lines = [];
        PrometheusMetrics.appendToolQuotaMetrics(lines, "codex_cli", [
            { kind: "local", percentRemaining: 50, source: "self_tracked" },
            { kind: "broken", percentRemaining: "not-a-number", source: "browser_sync" }
        ]);

        compare(lines.length, 0);
    }

    function test_missingResetAndEscapedLabels() {
        var lines = [];
        PrometheusMetrics.appendToolQuotaMetrics(lines, "tool\\name", [{
            kind: "quoted\"window",
            percentRemaining: 25,
            source: "antigravity_local",
            precision: "local_daemon_actual"
        }]);

        compare(lines.length, 1);
        compare(lines[0], "ai_usage_tool_quota_percent_remaining{tool=\"tool\\\\name\",kind=\"quoted\\\"window\",source=\"antigravity_local\",quality=\"local_daemon_actual\"} 25");
    }
}
