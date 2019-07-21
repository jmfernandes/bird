/*
 * This file is part of libphidget22
 *
 * Copyright 2015 Phidgets Inc <patrick@phidgets.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>
 */

#include "phidgetbase.h"
#include "constantsinternal.h"
#include "constants.h"

#define LIBRARY_VERSION "1.4"

#ifdef DEBUG
const char *LibraryVersion = "Phidget22 Debug - Version " LIBRARY_VERSION " - Built " __DATE__ " " __TIME__;
#else
const char *LibraryVersion = "Phidget22 - Version " LIBRARY_VERSION " - Built " __DATE__ " " __TIME__;
#endif
