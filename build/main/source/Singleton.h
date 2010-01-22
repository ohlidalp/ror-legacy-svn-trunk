/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

// damn you "using namespace Ogre"

template <class T>
class Singleton2
{
	static T* _self;
	static int _refcount;
protected:
	Singleton2(){}
	virtual ~Singleton2() { _self = 0; }
public:
	static T* Instance();
	static T* get();
	void FreeInstance();
};

template <class T>
T* Singleton2<T>::_self = 0;

template <class T>
int Singleton2<T>::_refcount = 0;

template <class T>
T* Singleton2<T>::get()
{
	return _self;
}

template <class T>
T* Singleton2<T>::Instance()
{
	if (!_self)
		_self = new T;
	_refcount++;
	return _self;
}

template <class T>
void Singleton2<T>::FreeInstance()
{
  if (--_refcount == 0)
    delete this;
}

#endif // __SINGLETON_H__
