'use client';

import React, { useEffect, useState } from 'react';
import { api } from '@/lib/api';
import { HistoryEvent } from '@/lib/types';
import { BarChart3, Layers, Clock, TrendingUp, RefreshCw } from 'lucide-react';

export default function ReportsPage() {
  const [history, setHistory] = useState<HistoryEvent[]>([]);
  const [summary, setSummary] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  const loadData = async () => {
    try {
      setLoading(true);
      const [histData, sumData] = await Promise.all([
        api.getHistory(),
        api.getSummary(),
      ]);
      setHistory(histData);
      setSummary(sumData);
    } catch (err: any) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const getEventBadge = (type: string) => {
    switch (type) {
      case 'ENTRY':
        return 'bg-emerald-950 text-emerald-400 border border-emerald-800';
      case 'EXIT':
        return 'bg-rose-950 text-rose-400 border border-rose-800';
      case 'RESERVATION':
        return 'bg-indigo-950 text-indigo-400 border border-indigo-800';
      case 'QUEUE_ALLOC':
      case 'QUEUE_JOIN':
        return 'bg-amber-950 text-amber-400 border border-amber-800';
      case 'PAYMENT':
        return 'bg-emerald-950 text-emerald-300 border border-emerald-800';
      default:
        return 'bg-slate-800 text-slate-300';
    }
  };

  return (
    <div className="space-y-8">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-2">
            <BarChart3 className="w-6 h-6 text-indigo-400" />
            Reports &amp; Historical Activity
          </h1>
          <p className="text-sm text-slate-400 mt-1">
            Real-time analytics and LIFO Stack inspection of operations and transactions.
          </p>
        </div>

        <button
          onClick={loadData}
          disabled={loading}
          className="flex items-center gap-2 px-3 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-xs font-semibold border border-slate-700 transition"
        >
          <RefreshCw className={`w-3.5 h-3.5 ${loading ? 'animate-spin' : ''}`} />
          Refresh
        </button>
      </div>

      {/* Analytics Summary Cards */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="text-xs text-slate-400">Total System Revenue</div>
          <div className="text-2xl font-bold text-emerald-400 mt-2">
            ₹{summary?.total_revenue?.toFixed(2) ?? '0.00'}
          </div>
          <div className="text-xs text-slate-500 mt-1">From verified payments</div>
        </div>

        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="text-xs text-slate-400">Current Facility Occupancy</div>
          <div className="text-2xl font-bold text-amber-400 mt-2">
            {summary?.occupancy_rate?.toFixed(1) ?? '0.0'}%
          </div>
          <div className="text-xs text-slate-500 mt-1">
            {summary?.occupied_slots ?? 0} occupied of {summary?.total_capacity ?? 0} total
          </div>
        </div>

        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="text-xs text-slate-400">Waiting Customers</div>
          <div className="text-2xl font-bold text-indigo-400 mt-2">
            {summary?.queue_count ?? 0}
          </div>
          <div className="text-xs text-slate-500 mt-1">In FIFO waiting queue</div>
        </div>
      </div>

      {/* LIFO History Stack Feed */}
      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-base font-semibold text-white flex items-center gap-2">
            <Layers className="w-5 h-5 text-indigo-400" />
            Recent Activity Stack Feed (LIFO - Top of Stack First)
          </h2>
          <span className="text-xs text-slate-400 font-mono">
            Synchronized with C11 History Stack &amp; DB logs
          </span>
        </div>

        <div className="space-y-2">
          {history.map((event, idx) => (
            <div
              key={`${event.timestamp}-${idx}`}
              className="p-4 bg-slate-900 border border-slate-800 rounded-xl flex flex-col sm:flex-row sm:items-center justify-between gap-3 hover:border-slate-700 transition"
            >
              <div className="flex items-center gap-3">
                <span className={`px-2.5 py-1 rounded-lg text-[10px] font-bold ${getEventBadge(event.event_type)}`}>
                  {event.event_type}
                </span>
                <span className="text-sm text-slate-200 font-medium">
                  {event.description}
                </span>
              </div>

              <div className="flex items-center gap-4 text-xs text-slate-400">
                {event.slot_number && (
                  <span className="font-mono text-indigo-300">
                    Slot: {event.slot_number}
                  </span>
                )}
                {event.license_plate && (
                  <span className="font-mono text-slate-300">
                    Plate: {event.license_plate}
                  </span>
                )}
                <span className="font-mono text-slate-500">
                  {new Date(event.timestamp * 1000).toLocaleTimeString()}
                </span>
              </div>
            </div>
          ))}

          {history.length === 0 && (
            <div className="p-8 text-center bg-slate-900 border border-slate-800 rounded-2xl text-slate-500 text-sm">
              No recent history events recorded on the stack yet.
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
