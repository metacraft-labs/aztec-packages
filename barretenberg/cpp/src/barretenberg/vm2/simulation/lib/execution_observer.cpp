#include "barretenberg/vm2/simulation/lib/execution_observer.hpp"

#include <utility>

namespace bb::avm2::simulation {

void ExecutionStepCollector::on_instruction(
    uint32_t context_id, const AztecAddress& contract_address, PC pc, WireOpCode opcode, const Gas& gas_used)
{
    steps.push_back(ExecutionStep{
        .context_id = context_id,
        .contract_address = contract_address,
        .pc = pc,
        .opcode = static_cast<uint8_t>(opcode),
        .gas_used = gas_used,
    });
}

std::vector<ExecutionStep> ExecutionStepCollector::dump_execution_steps()
{
    return std::move(steps);
}

} // namespace bb::avm2::simulation
