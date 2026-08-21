#pragma once

#include <vector>

#include "barretenberg/vm2/common/avm_io.hpp"
#include "barretenberg/vm2/common/aztec_types.hpp"
#include "barretenberg/vm2/common/memory_types.hpp"
#include "barretenberg/vm2/common/opcodes.hpp"

namespace bb::avm2::simulation {

// Per-instruction observation of the fast execution loop.
//
// HybridExecution::execute deliberately emits no ExecutionEvents — that is the point of the
// fast path — so there is currently no way to observe individual instructions without
// switching to Execution::execute and paying for hint collection. This interface is the
// seam: one virtual call per executed instruction, at the single place in the loop where an
// instruction has finished (or halted exceptionally).
//
// Shaped like CallStackMetadataCollectorInterface, which is injected the same way and gated
// by PublicSimulatorConfig::collect_call_metadata. The corresponding flag here is
// PublicSimulatorConfig::collect_execution_steps.
class ExecutionObserverInterface {
  public:
    virtual ~ExecutionObserverInterface() = default;

    // Called once per executed instruction, after dispatch, and also when the instruction
    // ended in an exceptional halt. `opcode` is LAST_OPCODE_SENTINEL if the halt happened
    // before the fetch completed. The opcode rather than the whole Instruction so that the
    // execution loop need only hoist trivially destructible state out of its try block;
    // operands remain recoverable from the bytecode at `pc`.
    virtual void on_instruction(
        uint32_t context_id, const AztecAddress& contract_address, PC pc, WireOpCode opcode, const Gas& gas_used) = 0;

    virtual std::vector<ExecutionStep> dump_execution_steps() = 0;
};

} // namespace bb::avm2::simulation
