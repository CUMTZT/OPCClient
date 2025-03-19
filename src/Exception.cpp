//
// Created by cumtzt on 25-3-19.
//
#include "Exception.h"

#include <utility>
Exception::Exception(int code): _pNested(nullptr), _code(code)
{
#ifdef ENABLE_TRACE
	std::ostringstream ostr;
	ostr << '\n';
	cpptrace::generate_trace(0,100).print(ostr);
	_msg = ostr.str();
#endif
}


Exception::Exception(std::string  msg, int code): _msg(std::move(msg)), _pNested(nullptr), _code(code)
{
#ifdef ENABLE_TRACE
	std::ostringstream ostr;
	ostr << '\n';
	cpptrace::generate_trace(0,100).print(ostr);
	_msg += ostr.str();
#endif
}


Exception::Exception(std::string  msg, const std::string& arg, int code): _msg(std::move(msg)), _pNested(nullptr), _code(code)
{
	if (!arg.empty())
	{
		_msg.append(": ");
		_msg.append(arg);
	}
#ifdef ENABLE_TRACE
	std::ostringstream ostr;
	ostr << '\n';
	cpptrace::generate_trace(0,100).print(ostr);
	_msg += ostr.str();
#endif
}


Exception::Exception(std::string  msg, const Exception& nested, int code): _msg(std::move(msg)), _pNested(nested.clone()), _code(code)
{
#ifdef ENABLE_TRACE
	std::ostringstream ostr;
	ostr << '\n';
	cpptrace::generate_trace(0,100).print(ostr);
	_msg += ostr.str();
#endif
}


Exception::Exception(const Exception& exc):
	std::exception(exc),
	_msg(exc._msg),
	_code(exc._code)
{
	_pNested = exc._pNested ? exc._pNested->clone() : nullptr;
}


Exception::~Exception() noexcept
{
	delete _pNested;
}


Exception& Exception::operator = (const Exception& exc)
{
	if (&exc != this)
	{
		Exception* newPNested = exc._pNested ? exc._pNested->clone() : nullptr;
		delete _pNested;
		_msg     = exc._msg;
		_pNested = newPNested;
		_code    = exc._code;
	}
	return *this;
}


const char* Exception::name() const noexcept
{
	return "Exception";
}


const char* Exception::className() const noexcept
{
	return typeid(*this).name();
}


const char* Exception::what() const noexcept
{
	return name();
}


std::string Exception::displayText() const
{
	std::string txt = name();
	if (!_msg.empty())
	{
		txt.append(": ");
		txt.append(_msg);
	}
	return txt;
}


void Exception::extendedMessage(const std::string& arg)
{
	if (!arg.empty())
	{
		if (!_msg.empty()) _msg.append(": ");
		_msg.append(arg);
	}
}


Exception* Exception::clone() const{
	return new Exception(*this);
}


void Exception::rethrow() const{
	throw *this;
}

IMPLEMENT_EXCEPTION(HttpException, Exception, "Http异常")
IMPLEMENT_EXCEPTION(HttpGetException, HttpException,"Http Get异常")
IMPLEMENT_EXCEPTION(HttpUpdateException, HttpGetException, "Http 更新请求异常")
IMPLEMENT_EXCEPTION(HttpUpdateNodeValueException, HttpUpdateException, "Http更新OPC节点值异常")
IMPLEMENT_EXCEPTION(HttpAddNodeException, HttpUpdateException, "Http增加OPC节点异常")
IMPLEMENT_EXCEPTION(HttpSearchException, HttpGetException,"Http查询异常")
IMPLEMENT_EXCEPTION(HttpGetNodesException, HttpSearchException,"Http查询OPC所有节点异常")