'use client';

import React, { useState } from 'react';
import { Slot, VehicleCategory, SlotStatus } from '@/lib/types';
import { Car, Bike, Zap, CheckCircle2, AlertCircle, Clock, ShieldAlert } from 'lucide-react';

interface SlotGridProps {
  slots: Slot[];
  onSelectSlot?: (slot: Slot) => void;
  selectedSlotId?: number | null;
  disableOccupied?: boolean;
}

export default function SlotGrid({
  slots,
  onSelectSlot,
  selectedSlotId,
  disableOccupied = false,
}: SlotGridProps) {
  const [filterCategory, setFilterCategory] = useState<VehicleCategory | 'ALL'>('ALL');

  const filteredSlots = slots.filter((s) => {
    if (filterCategory !== 'ALL' && s.category !== filterCategory) return false;
    return true;
  });

  const getStatusColor = (status: SlotStatus) => {
    switch (status) {
      case 'AVAILABLE':
        return 'bg-emerald-950/60 border-emerald-600/80 text-emerald-400 hover:border-emerald-400 hover:bg-emerald-900/60 cursor-pointer';
      case 'OCCUPIED':
        return disableOccupied
          ? 'bg-rose-950/40 border-rose-800/60 text-rose-400/60 cursor-not-allowed opacity-60'
          : 'bg-rose-950/60 border-rose-600/80 text-rose-400 hover:border-rose-400 hover:bg-rose-900/60 cursor-pointer';
      case 'RESERVED':
        return disableOccupied
          ? 'bg-amber-950/40 border-amber-800/60 text-amber-400/60 cursor-not-allowed opacity-60'
          : 'bg-amber-950/60 border-amber-600/80 text-amber-400 hover:border-amber-400 hover:bg-amber-900/60 cursor-pointer';
      default:
        return 'bg-slate-800 border-slate-700 text-slate-400 cursor-not-allowed';
    }
  };

  const getCategoryIcon = (cat: VehicleCategory) => {
    switch (cat) {
      case 'CAR':
        return <Car className="w-4 h-4" />;
      case 'BIKE':
        return <Bike className="w-4 h-4" />;
      case 'EV':
        return <Zap className="w-4 h-4 text-emerald-400" />;
    }
  };

  return (
    <div className="space-y-4">
      {/* Category Filter Pills & Legend */}
      <div className="flex flex-wrap items-center justify-between gap-4 p-4 bg-slate-900 border border-slate-800 rounded-xl">
        <div className="flex items-center gap-2">
          <span className="text-xs text-slate-400 font-medium">Filter Category:</span>
          {(['ALL', 'CAR', 'BIKE', 'EV'] as const).map((cat) => (
            <button
              key={cat}
              onClick={() => setFilterCategory(cat)}
              className={`px-3 py-1 rounded-lg text-xs font-semibold transition ${
                filterCategory === cat
                  ? 'bg-indigo-600 text-white'
                  : 'bg-slate-800 text-slate-400 hover:bg-slate-700 hover:text-white'
              }`}
            >
              {cat}
            </button>
          ))}
        </div>

        {/* Legend */}
        <div className="flex items-center gap-4 text-xs">
          <div className="flex items-center gap-1.5">
            <span className="w-3 h-3 rounded bg-emerald-500" />
            <span className="text-slate-300">Available</span>
          </div>
          <div className="flex items-center gap-1.5">
            <span className="w-3 h-3 rounded bg-rose-500" />
            <span className="text-slate-300">Occupied (Click for details)</span>
          </div>
          <div className="flex items-center gap-1.5">
            <span className="w-3 h-3 rounded bg-amber-500" />
            <span className="text-slate-300">Reserved (Click for details)</span>
          </div>
        </div>
      </div>

      {/* 2D Parking Slots Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-6 lg:grid-cols-7 gap-3">
        {filteredSlots.map((slot) => {
          const isSelected = selectedSlotId === slot.slot_id;
          const isClickDisabled = disableOccupied && slot.status !== 'AVAILABLE';

          return (
            <button
              key={slot.slot_id}
              onClick={() => onSelectSlot && !isClickDisabled && onSelectSlot(slot)}
              disabled={isClickDisabled}
              title={`Slot ${slot.slot_number} - Click to view full details`}
              className={`p-3 rounded-xl border flex flex-col justify-between h-24 transition text-left relative group ${getStatusColor(
                slot.status
              )} ${isSelected ? 'ring-2 ring-indigo-400 shadow-lg shadow-indigo-500/30' : ''}`}
            >
              <div className="flex items-center justify-between w-full">
                <span className="text-xs font-mono font-bold tracking-wider">{slot.slot_number}</span>
                <span className="p-1 rounded bg-slate-900/60">{getCategoryIcon(slot.category)}</span>
              </div>

              {/* Middle preview badge */}
              <div className="my-auto truncate">
                {slot.status === 'OCCUPIED' && (
                  <span className="inline-block truncate max-w-full text-[9px] font-mono font-semibold bg-rose-950/90 text-rose-200 border border-rose-800/80 px-1.5 py-0.5 rounded shadow-sm">
                    {slot.license_plate || 'PARKED'}
                  </span>
                )}
                {slot.status === 'RESERVED' && (
                  <span className="inline-block truncate max-w-full text-[9px] font-mono font-semibold bg-amber-950/90 text-amber-200 border border-amber-800/80 px-1.5 py-0.5 rounded shadow-sm">
                    {slot.customer_name || 'RESERVED'}
                  </span>
                )}
                {slot.status === 'AVAILABLE' && (
                  <span className="text-[10px] text-emerald-400/80 font-medium">
                    ₹{slot.hourly_rate ?? (slot.category === 'BIKE' ? 100 : slot.category === 'EV' ? 200 : 250)}/hr
                  </span>
                )}
              </div>

              <div className="flex items-center justify-between w-full">
                <span className="text-[10px] font-semibold tracking-wide uppercase">{slot.status}</span>
                <span className="text-[10px] text-slate-400">{slot.category}</span>
              </div>
            </button>
          );
        })}
      </div>
    </div>
  );
}
