'use client';

import React from 'react';
import Link from 'next/link';
import { usePathname } from 'next/navigation';
import {
  LayoutDashboard,
  CalendarCheck,
  DoorOpen,
  Users,
  Clock,
  Receipt,
  BarChart3,
  Settings,
} from 'lucide-react';

const NAV_ITEMS = [
  { name: 'Dashboard', href: '/', icon: LayoutDashboard },
  { name: 'Reserve Slot', href: '/reserve', icon: CalendarCheck },
  { name: 'Gate Terminal', href: '/gate', icon: DoorOpen },
  { name: 'Waiting Queue', href: '/queue', icon: Clock },
  { name: 'Customers & Cars', href: '/customers', icon: Users },
  { name: 'Billing & Invoices', href: '/billing', icon: Receipt },
  { name: 'Reports & Logs', href: '/reports', icon: BarChart3 },
];

export default function Sidebar() {
  const pathname = usePathname();

  return (
    <aside className="w-64 bg-slate-900 border-r border-slate-800 flex flex-col justify-between p-4 min-h-[calc(100vh-4rem)]">
      <nav className="space-y-1">
        <div className="px-3 py-2 text-xs font-semibold text-slate-500 uppercase tracking-wider">
          Management
        </div>
        {NAV_ITEMS.map((item) => {
          const Icon = item.icon;
          const isActive = pathname === item.href;
          return (
            <Link
              key={item.href}
              href={item.href}
              className={`flex items-center gap-3 px-3 py-2.5 rounded-lg text-sm font-medium transition ${
                isActive
                  ? 'bg-indigo-600 text-white shadow-md shadow-indigo-600/30'
                  : 'text-slate-400 hover:text-white hover:bg-slate-800/60'
              }`}
            >
              <Icon className="w-4 h-4" />
              {item.name}
            </Link>
          );
        })}
      </nav>

      <div className="p-3 bg-slate-800/50 rounded-xl border border-slate-800">
        <div className="text-xs text-slate-400 font-medium">Architecture</div>
        <div className="text-xs text-slate-300 mt-1">
          Next.js Client ⇄ C11 REST Daemon ⇄ SQLite3 Engine
        </div>
      </div>
    </aside>
  );
}
