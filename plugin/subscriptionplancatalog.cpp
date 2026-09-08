#include "subscriptionplancatalog.h"

#include <QJsonArray>
#include <QUrl>
#include <cmath>

SubscriptionPlanCatalog::SubscriptionPlanCatalog(QObject *parent)
    : CatalogLoader(QStringLiteral("subscriptions-v1.json"), 1, parent)
{
    load();
}

SubscriptionPlanCatalog *SubscriptionPlanCatalog::instance()
{
    static SubscriptionPlanCatalog catalog;
    return &catalog;
}

QVariantList SubscriptionPlanCatalog::tools() const
{
    QVariantList result;
    const QJsonArray tools = rootObject().value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &entry : tools) {
      result << tool(entry.toObject().value(QStringLiteral("key")).toString());
    }
    return result;
}

QVariantMap SubscriptionPlanCatalog::tool(const QString &toolKey) const
{
  QVariantMap result = toolObject(toolKey).toVariantMap();
  QVariantList plans;
  for (const QString &id : planIdsForTool(toolKey))
    plans.append(plan(toolKey, id));
  result.insert("plans", plans);
  QVariantList modes;
  for (const QVariant &value : result.value("billingModes").toList())
    modes.append(billingMode(toolKey, value.toMap().value("id").toString()));
  result.insert("billingModes", modes);
  return result;
}

QStringList
SubscriptionPlanCatalog::planIdsForTool(const QString &toolKey) const {
  QStringList result;
  const QJsonArray plans =
      toolObject(toolKey).value(QStringLiteral("plans")).toArray();
  for (const QJsonValue &entry : plans) {
    const QString id = entry.toObject().value(QStringLiteral("id")).toString();
    if (!id.isEmpty()) {
      result << id;
    }
  }
  return result;
}

QStringList
SubscriptionPlanCatalog::planLabelsForTool(const QString &toolKey) const {
  QStringList result;
  const QJsonArray plans =
      toolObject(toolKey).value(QStringLiteral("plans")).toArray();
  for (const QJsonValue &entry : plans) {
    const QString label =
        entry.toObject().value(QStringLiteral("label")).toString();
    if (!label.isEmpty()) {
      result << label;
    }
  }
  return result;
}

QString
SubscriptionPlanCatalog::planIdForLabel(const QString &toolKey,
                                        const QString &planLabelOrId) const {
  const QString normalized = planLabelOrId.trimmed().toLower();
  const QJsonArray plans =
      toolObject(toolKey).value(QStringLiteral("plans")).toArray();
  if (normalized.isEmpty() && !plans.isEmpty()) {
    return plans.first().toObject().value(QStringLiteral("id")).toString();
  }
  for (const QJsonValue &entry : plans) {
    const QJsonObject plan = entry.toObject();
    if (plan.value(QStringLiteral("id")).toString().toLower() == normalized ||
        plan.value(QStringLiteral("label")).toString().toLower() ==
            normalized) {
      return plan.value(QStringLiteral("id")).toString();
    }
  }
  return QString();
}

QString
SubscriptionPlanCatalog::planLabelForId(const QString &toolKey,
                                        const QString &planIdOrLabel) const {
  const QString normalized = planIdOrLabel.trimmed().toLower();
  const QJsonArray plans =
      toolObject(toolKey).value(QStringLiteral("plans")).toArray();
  if (normalized.isEmpty() && !plans.isEmpty()) {
    return plans.first().toObject().value(QStringLiteral("label")).toString();
  }
  for (const QJsonValue &entry : plans) {
    const QJsonObject plan = entry.toObject();
    if (plan.value(QStringLiteral("id")).toString().toLower() == normalized ||
        plan.value(QStringLiteral("label")).toString().toLower() ==
            normalized) {
      return plan.value(QStringLiteral("label")).toString();
    }
  }
  return QString();
}

QVariantMap SubscriptionPlanCatalog::plan(const QString &toolKey,
                                          const QString &planIdOrLabel) const {
  QVariantMap result = planObject(toolKey, planIdOrLabel).toVariantMap();
  if (result.isEmpty())
    return result;
  result.insert("price", price(toolKey, planIdOrLabel));
  result.insert("quotaWindows", quotaWindows(toolKey, planIdOrLabel));
  return result;
}

QVariantList
SubscriptionPlanCatalog::quotaWindows(const QString &toolKey,
                                      const QString &planIdOrLabel) const {
  QVariantList result;
  const QJsonArray windows = planObject(toolKey, planIdOrLabel)
                                 .value(QStringLiteral("quotaWindows"))
                                 .toArray();
  for (const QJsonValue &entry : windows) {
    result << evaluateEvidence(entry.toObject().toVariantMap(),
                               QDate::currentDate());
  }
  return result;
}

QVariantMap SubscriptionPlanCatalog::price(const QString &toolKey,
                                           const QString &planIdOrLabel) const {
  return evaluateEvidence(planObject(toolKey, planIdOrLabel)
                              .value(QStringLiteral("price"))
                              .toObject()
                              .toVariantMap(),
                          QDate::currentDate());
}

QVariantMap SubscriptionPlanCatalog::billingMode(const QString &toolKey,
                                                 const QString &modeId) const {
  const QString normalized = modeId.trimmed().toLower();
  const QJsonArray modes =
      toolObject(toolKey).value(QStringLiteral("billingModes")).toArray();
  for (const QJsonValue &entry : modes) {
    const QJsonObject mode = entry.toObject();
    if (mode.value(QStringLiteral("id")).toString().toLower() == normalized) {
      QVariantMap result = mode.toVariantMap();
      result.insert("quotaWindows", billingModeQuotaWindows(toolKey, modeId));
      return result;
    }
  }
  return QVariantMap();
}

QVariantList
SubscriptionPlanCatalog::billingModeQuotaWindows(const QString &toolKey,
                                                 const QString &modeId) const {
  QVariantList result;
  const QString normalized = modeId.trimmed().toLower();
  const QJsonArray modes =
      toolObject(toolKey).value(QStringLiteral("billingModes")).toArray();
  QJsonArray windows;
  for (const QJsonValue &entry : modes) {
    const QJsonObject mode = entry.toObject();
    if (mode.value(QStringLiteral("id")).toString().toLower() == normalized) {
      windows = mode.value(QStringLiteral("quotaWindows")).toArray();
      break;
    }
  }
  for (const QJsonValue &entry : windows) {
    result << evaluateEvidence(entry.toObject().toVariantMap(),
                               QDate::currentDate());
  }
  return result;
}

QJsonObject SubscriptionPlanCatalog::toolObject(const QString &toolKey) const
{
    const QString normalized = toolKey.trimmed().toLower();
    const QJsonArray tools = rootObject().value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &entry : tools) {
        const QJsonObject tool = entry.toObject();
        if (tool.value(QStringLiteral("key")).toString().toLower() == normalized) {
            return tool;
        }
    }
    return QJsonObject();
}

QJsonObject
SubscriptionPlanCatalog::planObject(const QString &toolKey,
                                    const QString &planIdOrLabel) const {
  const QString normalized = planIdOrLabel.trimmed().toLower();
  const QJsonArray plans =
      toolObject(toolKey).value(QStringLiteral("plans")).toArray();
  if (plans.isEmpty()) {
    return QJsonObject();
  }

  if (normalized.isEmpty()) {
    return plans.first().toObject();
  }

  for (const QJsonValue &entry : plans) {
    const QJsonObject plan = entry.toObject();
    if (plan.value(QStringLiteral("id")).toString().toLower() == normalized ||
        plan.value(QStringLiteral("label")).toString().toLower() ==
            normalized) {
      return plan;
    }
  }

  return QJsonObject();
}

QVariantMap SubscriptionPlanCatalog::evaluateEvidence(QVariantMap entry,
                                                      const QDate &today) {
  if (entry.isEmpty())
    return entry;
  const QVariantMap evidence = entry.value("evidence").toMap();
  const QDate reviewed =
      QDate::fromString(evidence.value("reviewedAt").toString(), Qt::ISODate);
  const QDate effective = QDate::fromString(
      evidence.value("effectiveFrom").toString(), Qt::ISODate);
  const QDate expires =
      QDate::fromString(evidence.value("expiresAt").toString(), Qt::ISODate);
  QString state = QStringLiteral("current");
  bool validSources = !evidence.value("sourceRefs").toList().isEmpty();
  for (const QVariant &ref : evidence.value("sourceRefs").toList()) {
    const QVariantMap source = ref.toMap();
    const QUrl url(source.value("url").toString());
    const QDate sourceDate =
        QDate::fromString(source.value("reviewedAt").toString(), Qt::ISODate);
    validSources &= url.scheme() == "https" && !url.host().isEmpty() &&
                    !source.value("label").toString().isEmpty() &&
                    sourceDate.isValid() && sourceDate <= reviewed &&
                    sourceDate.daysTo(expires) <= 30;
  }
  bool validNumbers = true;
  for (const QString &key :
       {QStringLiteral("amount"), QStringLiteral("rangeMin"),
        QStringLiteral("rangeMax"), QStringLiteral("limit"),
        QStringLiteral("creditUsdValue")}) {
    if (!entry.contains(key))
      continue;
    const QVariant value = entry.value(key);
    const bool numeric = value.metaType().id() == QMetaType::Double ||
                         value.metaType().id() == QMetaType::Int ||
                         value.metaType().id() == QMetaType::LongLong;
    validNumbers &=
        numeric && std::isfinite(value.toDouble()) && value.toDouble() >= 0;
  }
  if (entry.contains("rangeMin") || entry.contains("rangeMax")) {
    validNumbers &= entry.contains("rangeMin") && entry.contains("rangeMax") &&
                    !entry.contains("amount") &&
                    entry.value("rangeMin").toDouble() <=
                        entry.value("rangeMax").toDouble() &&
                    entry.value("precision").toString() != "official_exact";
  }
  if (evidence.isEmpty())
    state = QStringLiteral("unreviewed");
  else if (!today.isValid() || !reviewed.isValid() || !effective.isValid() ||
           !expires.isValid() || expires < reviewed || expires < effective ||
           reviewed.daysTo(expires) > 30 || !validSources || !validNumbers)
    state = QStringLiteral("invalid");
  else if (today < effective || today < reviewed)
    state = QStringLiteral("not_yet_effective");
  else if (today > expires)
    state = QStringLiteral("expired");
  else if (entry.value("precision").toString() == "needs_manual_review" ||
           evidence.value("needsManualReview").toBool() ||
           evidence.value("archived").toBool())
    state = QStringLiteral("unreviewed");
  entry.insert("evidenceState", state);
  entry.insert("available", state == "current");
  if (state != "current") {
    for (const QString &key :
         {QStringLiteral("amount"), QStringLiteral("rangeMin"),
          QStringLiteral("rangeMax"), QStringLiteral("limit"),
          QStringLiteral("creditUsdValue")})
      entry.remove(key);
    entry.insert("precision", QStringLiteral("needs_manual_review"));
  }
  return entry;
}

QVariantList SubscriptionPlanCatalog::evidenceReviewItems() const {
  QVariantList result;
  for (const QJsonValue &toolValue : rootObject().value("tools").toArray()) {
    const QJsonObject tool = toolValue.toObject();
    for (const QString &group :
         {QStringLiteral("plans"), QStringLiteral("billingModes")}) {
      for (const QJsonValue &planValue : tool.value(group).toArray()) {
        const QJsonObject plan = planValue.toObject();
        QVariantList entries =
            plan.value("quotaWindows").toArray().toVariantList();
        if (plan.contains("price"))
          entries.append(plan.value("price").toObject().toVariantMap());
        for (const QVariant &value : entries) {
          const QVariantMap entry =
              evaluateEvidence(value.toMap(), QDate::currentDate());
          if (entry.value("available").toBool() ||
              entry.value("evidence").toMap().value("archived").toBool())
            continue;
          result.append(QVariantMap{
              {"key", tool.value("key").toString()},
              {"planId", plan.value("id").toString()},
              {"label",
               entry.value("label", QStringLiteral("Subscription price"))},
              {"reviewReason",
               entry.value("evidenceState").toString() == "unreviewed" &&
                       !tool.value("reviewReason").toString().isEmpty()
                   ? tool.value("reviewReason").toString()
                   : entry.value("evidenceState").toString()},
              {"sourceRefs",
               entry.value("evidence").toMap().value("sourceRefs")}});
        }
      }
    }
  }
  return result;
}
