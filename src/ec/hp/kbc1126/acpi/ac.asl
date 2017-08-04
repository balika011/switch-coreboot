/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Iru Cai <mytbk920423@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

Name (ACST, 0x01)
Name (SMAR, 0x00)

Field (ECRM, ByteAcc, NoLock, Preserve)
{
	Offset (0x84),
	ADP,    1,
	ADID,   2
}

Method (UPAD, 0, Serialized)
{
	Acquire (BTMX, 0xFFFF)
	If ((GACP & 0x01))
	{
		GACP &= 0x06
		Release (BTMX)
		Acquire (ECMX, 0xFFFF)
		Local0 = ADP
		Local1 = ADID
		Release (ECMX)
		ACST = Local0
		SMAR = Local1
	}
	Else
	{
		Release (BTMX)
	}
}

Method (GPID, 0, Serialized)
{
	UPAD ()
	Return (SMAR)
}

Method (GACS, 0, Serialized) /* get AC status */
{
	UPAD ()
	Return (ACST)
}

Device (AC)
{
	Name (_HID, "ACPI0003")
	Name (_PCL, Package () { \_SB })

	Method (_STA)
	{
		Return (0x0F)
	}

	Method (_PSR, 0, NotSerialized)
	{
		Local0 = GACS ()
		PWRS = Local0 /* GNVS.PWRS */
		Local1 = GPID ()
		ACST = Local0
		SMAR = Local1
		Return (Local0)
	}
}

Method (_Q06, 0, NotSerialized)
{
	Store ("EC: AC STATUS", Debug)
	PWUP (0x05, (0x02 | 0x01))
	If (BTDR (0x02))
	{
		Notify (AC, 0x80)
		PNOT ()
	}
}