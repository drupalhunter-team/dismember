#include "dsmem_trace.h"
#include "memlocdata.h"
#include "instruction.h"
#include "binaryconstant.h"
#include "datatype.h"
#include "symbol_analysis.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

sp_RunQueueJob SymbolAnalysis::createAnalysisJob(Document * d)
{
	FunctorRunQueueJob::jobfun_t jb =
			boost::bind(&SymbolAnalysis::analyze, d);
	return createFunctorRunQueueJob("symbol analysis..", jb);
}

void SymbolAnalysis::submitAnalysisJob(Document * d)
{
	d->getRunQueue()->submit(createAnalysisJob(d));
}

bool SymbolAnalysis::analyze(Document *d)
{
	Trace *t = d->getTrace();
	char namebuf[128];
	char type[16];
	int size;
	bool subroutine = false;

	MemlocManager::memloclist_ci it = t->memloc_list_begin();
	MemlocManager::memloclist_ci end = t->memloc_list_end();
	
	for (; it != end; ++it) {
		MemlocData * id = (*it).second;
		if (!id || !id->has_xrefs_to())
			continue;
		if (id->get_symbol()) {
			const Symbol *sym = id->get_symbol();
			AbstractData *ad = sym->get_property("generated");
			if (!ad || !boost::get<bool>(*ad))
				continue;
		}

		Instruction * aid = dynamic_cast<Instruction *>(id);

		if (!id->is_executable()) {
			sprintf(type, "data");
			u32 addr = (u32)id->get_addr();
			switch ((size = id->get_length())) {
			case 4:
				sprintf(namebuf, "_dword_%08X", addr);
				break;
			case 2:
				sprintf(namebuf, "_hword_%08X", addr);
				break;
			case 1:
				sprintf(namebuf, "_byte_%08X", addr);
				break;
			}
		}
		else if (aid && aid->get_pcflags() & Instruction::PCFLAG_FNENT) {
			sprintf(type, "code");
			subroutine = true;
			sprintf(namebuf, "_sub_%08X", (u32)id->get_addr());
		}
		else {
			sprintf(type, "unk");
			sprintf(namebuf, "_loc_%08X", (u32)id->get_addr());
		}
		if (id->get_symbol() && !strcmp(id->get_symbol()->get_name().c_str(), namebuf))
			continue;
		
		Symbol *sym = t->create_sym(namebuf, id->get_addr());
		sym->set_property("type", new AbstractData(std::string(type)));
		if (!id->is_executable())
			sym->set_property("size", new AbstractData(size));
		sym->set_property("subroutine", new AbstractData(subroutine));
		sym->set_property("generated", new AbstractData(true));
	}

	d->postGuiUpdate();
	return true;
}
