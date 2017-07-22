#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class noncopyable
{
protected:
	noncopyable() {};
	virtual ~noncopyable() {};
private:
	noncopyable(const noncopyable&);
	const noncopyable & operator= (const noncopyable &);
};

#endif /* NONCOPYABLE_H */

