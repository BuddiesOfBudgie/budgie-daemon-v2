#include "DisplayService.hpp"

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
    for (const auto& output : WaylandOrchestrator::instance().getManager()->getHeads()) {
      outputs.append(output->getIdentifier());
    }
    return outputs;
  }

  DisplaysAdaptor* DisplayService::GetAdaptor() {
    return m_adaptor;
  }
}
