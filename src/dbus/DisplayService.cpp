#include "DisplayService.hpp"

#include "displays/batch-system/ConfigurationBatchSystem.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"

namespace bd {
  DisplayService::DisplayService(QObject* parent) : QObject(parent) {
    m_adaptor = new DisplaysAdaptor(this);
  }

  DisplayService& DisplayService::instance() {
    static DisplayService _instance(nullptr);
    return _instance;
  }

  QStringList DisplayService::GetAvailableOutputs() {
    auto outputs = QStringList {};
    for (const auto& output : WaylandOrchestrator::instance().getManager()->getHeads()) { outputs.append(output->getIdentifier()); }
    return outputs;
  }

  static QSharedPointer<WaylandOutputMetaHead> getPrimaryOrFirstHead() {
    auto manager = WaylandOrchestrator::instance().getManager();
    if (!manager) return nullptr;
    const auto heads = manager->getHeads();
    if (heads.isEmpty()) return nullptr;

    for (const auto& head : heads) {
      if (head && head->isPrimary()) return head;
    }
    return heads.first();
  }

  QString DisplayService::GetPrimaryOutput() {
    auto head = getPrimaryOrFirstHead();
    if (!head) return QString();
    return head->getIdentifier();
  }

  QVariantMap DisplayService::GetPrimaryOutputRect() {
    QVariantMap rect;
    auto        head = getPrimaryOrFirstHead();
    if (!head) return rect;

    // Populate QRect-like map similar to GetModeInfo pattern
    int  x    = head->getPosition().x();
    int  y    = head->getPosition().y();
    int  w    = 0;
    int  h    = 0;
    auto mode = head->getCurrentMode();
    if (mode) {
      auto sizeOpt = mode->getSize();
      if (sizeOpt.has_value()) {
        w = sizeOpt->width();
        h = sizeOpt->height();
      }
    }

    rect["X"]      = x;
    rect["Y"]      = y;
    rect["Width"]  = w;
    rect["Height"] = h;
    return rect;
  }

  QVariantMap DisplayService::GetGlobalRect() {
    QVariantMap rect;
    auto calculationResult = ConfigurationBatchSystem::instance().getCalculationResult();
    if (!calculationResult) return rect;

    auto globalSpace = calculationResult->getGlobalSpace();
    if (!globalSpace) return rect;

    rect["X"] = globalSpace->x();
    rect["Y"] = globalSpace->y();
    rect["Width"] = globalSpace->width();
    rect["Height"] = globalSpace->height();
    return rect;
  }

  DisplaysAdaptor* DisplayService::GetAdaptor() {
    return m_adaptor;
  }
}
