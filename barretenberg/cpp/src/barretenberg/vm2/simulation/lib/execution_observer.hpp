#pragma once

#include <vector>

#include "barretenberg/vm2/common/avm_io.hpp"
#include "barretenberg/vm2/simulation/interfaces/execution_observer.hpp"

namespace bb::avm2::simulation {

// Records one ExecutionStep per executed instruction. Installed only when
// PublicSimulatorConfig::collect_execution_steps is set.
class ExecutionStepCollector : public ExecutionObserverInterface {
  public:
    void on_instruction(uint32_t context_id,
                        const AztecAddress& contract_address,
                        PC pc,
                        WireOpCode opcode,
                        const Gas& gas_used) override;
    std::vector<ExecutionStep> dump_execution_steps() override;

  private:
    std::vector<ExecutionStep> steps;
};

} // namespace bb::avm2::simulation
