/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "codeblock.h"
#include "i_memlocdata.h"
#include "instruction.h"
#include "xref.h"
#include "xrefmanager.h"

CodeBlock::CodeBlock(I_ProjectModel * t, address_t start, address_t end) :  m_start(start), m_end(end), m_trace(t)
{
	
}

void CodeBlockManager::linkCodeBlocks(CodeBlock * pre, CodeBlock * post)
{
	post->m_pre.insert(pre);
	pre->m_post.insert(post);
}

void CodeBlockManager::unlinkCodeBlocks(CodeBlock * pre, CodeBlock * post)
{
	post->m_pre.erase(pre);
	pre->m_post.erase(post);
}

bool hasNonLinkCodeXref(XrefManager::xref_map_ci begin, XrefManager::xref_map_ci end)
{

	
	for (XrefManager::xref_map_ci i=begin; i != end; i++)
	{
		Xref * xr = (*i).second;
		uint32_t type = xr->get_type();
		if ((type == Xref::XR_TYPE_JMP))
			return true;
	}
	return false;
}

CodeBlock * CodeBlockManager::buildCodeBlockAt(I_MemlocData * growFrom)
{
	I_MemlocData * current_start = growFrom;
	I_MemlocData * current_start_prev = growFrom;
	I_MemlocData * current_end = growFrom;
	I_MemlocData * current_end_next = growFrom;
	
	if (!dynamic_cast<Instruction *>(growFrom))
		return NULL;
		
	// while we're not at the beginning of the memory segment,
	// and while we don't have any code xrefs to this location,
	// we need to continue to loop
	// We may break out of the loop if the insn before has an xref from that is code 
	while ((current_start_prev = dynamic_cast<Instruction *>(current_start->getPreviousContiguous())) &&
		   !hasNonLinkCodeXref(current_start->begin_xref_to(), current_start->end_xref_to()) &&
		   !hasNonLinkCodeXref(current_start_prev->begin_xref_from(), current_start->begin_xref_from())
		   )
		current_start = current_start_prev;

	
	
	while ((current_end_next = dynamic_cast<Instruction *>(current_end->getNextContiguous())) &&
		   !hasNonLinkCodeXref(current_end_next->begin_xref_to(), current_end_next->end_xref_to()) &&
		   !hasNonLinkCodeXref(current_end->begin_xref_from(), current_end->begin_xref_from())
		   )
		current_end = current_end_next;
	
	CodeBlock * cb = new CodeBlock(m_trace, current_start->get_addr(), current_end->get_addr());
	
	m_blocks[cb->getStart()] = cb;
	return cb;
}

void CodeBlockManager::codeBlockAnalysisPassAt( address_t growFrom)
{
	if (growFrom == address_t())
	{
		return;
	}
	
	buildCodeBlockAt(m_trace->lookup_memloc(growFrom,false));
}

CodeBlock * CodeBlockManager::codeBlockAt(address_t addr)
{
	blockiterator i = m_blocks.lower_bound(addr);
	if (i == m_blocks.begin())
		return NULL;
	
	i--;
	
	return (*i).second;
}
