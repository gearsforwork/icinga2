/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga/checkresult.hpp"
#include "icinga/checkresult.tcpp"
#include "icinga/checkcommand.hpp"
#include "icinga/perfdatavalue.hpp"
#include "base/scriptglobal.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

REGISTER_TYPE(CheckResult);
INITIALIZE_ONCE(&CheckResult::StaticInitialize);

void CheckResult::StaticInitialize(void)
{
	ScriptGlobal::Set("ServiceOK", ServiceOK);
	ScriptGlobal::Set("ServiceWarning", ServiceWarning);
	ScriptGlobal::Set("ServiceCritical", ServiceCritical);
	ScriptGlobal::Set("ServiceUnknown", ServiceUnknown);

	ScriptGlobal::Set("HostUp", HostUp);
	ScriptGlobal::Set("HostDown", HostDown);
}

double CheckResult::CalculateExecutionTime(void) const
{
	return GetExecutionEnd() - GetExecutionStart();
}

double CheckResult::CalculateLatency(void) const
{
	double latency = (GetScheduleEnd() - GetScheduleStart()) - CalculateExecutionTime();

	if (latency < 0)
		latency = 0;

	return latency;
}

Array::Ptr CheckResult::GetPerformanceDataParsed(const Checkable::Ptr& checkable) const
{
	Array::Ptr result = new Array();

	Array::Ptr perfdata = GetPerformanceData();

	if (!perfdata)
		return result;

	Function::Ptr handler = checkable->GetCheckCommand()->GetPerfdataHandler();

	ObjectLock olock(perfdata);
	BOOST_FOREACH(const Value& val, perfdata) {
		PerfdataValue::Ptr pdv;

		if (val.IsObjectType<PerfdataValue>())
			pdv = val;
		else {
			try {
				pdv = PerfdataValue::Parse(val);
			} catch (const std::exception&) {
				result->Add(val);
				continue;
			}
		}
	}

	if (handler) {
		ScriptFrame frame;
		std::vector<Value> args;
		args.push_back(checkable);
		args.push_back(result);
		handler->Invoke(args);
	}

	return result;
}
