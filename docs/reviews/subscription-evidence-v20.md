# V20 subscription evidence lifecycle

The existing September 5 source review remains the source of reviewedAt values;
this migration does not claim a fresh review of every vendor. Every price and
allowance now embeds those supporting references with its own effective date and
30-day expiry. Effective dates denote when the reviewed snapshot can be used,
not an invented historical launch date. Archived Copilot request records retain
the actual September review date and remain unavailable for current billing.

On September 7 the official [JetBrains plans and usage](https://www.jetbrains.com/help/ai-assistant/licensing-and-subscriptions.html)
page was independently read back. It still distinguishes personal and commercial
prices and credit amounts, and documents pooled organizational resources.
Existing ranges are retained; they are not a bill or an individual entitlement.
No account or authenticated quota was tested during this catalog review.

Windsurf evidence remains under manual review using the references in the
[September review](subscription-catalog-2026-09-05.md). Its numeric public prices
are withheld at runtime while account transitions remain unresolved.

The scheduled catalog workflow emits a subscription report alongside the provider
report. Each actionable row identifies its plan, reason, and official references.
No runtime web scraping is added. Structural validation uses --structural-only;
release and scheduled checks retain real-date freshness enforcement. Synthetic
unit tests pin dates and cover expiry boundaries and malformed ranges.
