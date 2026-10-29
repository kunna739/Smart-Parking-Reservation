'use client';

import React, { useEffect, useState, Suspense } from 'react';
import { useSearchParams } from 'next/navigation';
import { api } from '@/lib/api';
import { Slot } from '@/lib/types';
import {
  Receipt,
  Car,
  Bike,
  Zap,
  Printer,
  CheckCircle2,
  Banknote,
  QrCode,
  CreditCard,
  User,
  Phone,
  Clock,
  Search,
  Sparkles,
  ArrowRight,
  RefreshCw,
} from 'lucide-react';

function BillingContent() {
  const searchParams = useSearchParams();

  const [summary, setSummary] = useState<any>(null);
  const [occupiedSlots, setOccupiedSlots] = useState<Slot[]>([]);
  const [selectedPlate, setSelectedPlate] = useState<string>('');
  const [activeSlot, setActiveSlot] = useState<Slot | null>(null);

  // Billing form state
  const [amount, setAmount] = useState<string>('250.00');
  const [paymentMethod, setPaymentMethod] = useState<'CASH' | 'UPI' | 'CARD'>('CASH');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Completed Receipt state
  const [receipt, setReceipt] = useState<{
    payment_id: number;
    license_plate: string;
    driver_name: string;
    phone: string;
    slot_number: string;
    category: string;
    amount: number;
    method: string;
    timestamp: string;
  } | null>(null);

  const loadData = async () => {
    try {
      const [sumData, slotsData] = await Promise.all([
        api.getSummary(),
        api.getSlots(),
      ]);
      setSummary(sumData);

      // Filter only slots that currently have a vehicle parked or reserved
      const active = slotsData.filter(
        (s) => (s.status === 'OCCUPIED' || s.status === 'RESERVED') && s.license_plate
      );
      setOccupiedSlots(active);

      // If URL params provided (e.g. redirected from exit gate)
      const plateParam = searchParams.get('plate');
      const amountParam = searchParams.get('amount');

      if (plateParam) {
        setSelectedPlate(plateParam);
        if (amountParam) {
          setAmount(parseFloat(amountParam).toFixed(2));
        }

        const match = active.find(
          (s) => s.license_plate?.toUpperCase() === plateParam.trim().toUpperCase()
        );
        if (match) {
          setActiveSlot(match);
          if (!amountParam && match.estimated_fee) {
            setAmount(match.estimated_fee.toFixed(2));
          }
        } else {
          // If already checked out, lookup vehicle history / customer name via search
          try {
            const vInfo = await api.searchVehicle(plateParam);
            if (vInfo) {
              setActiveSlot({
                slot_id: vInfo.slot_id || 0,
                slot_number: vInfo.slot_number || 'Exited Gate',
                category: vInfo.category || 'CAR',
                status: 'AVAILABLE',
                license_plate: vInfo.license_plate,
                customer_name: vInfo.customer_name,
                customer_phone: '',
                estimated_fee: amountParam ? parseFloat(amountParam) : 250.0,
              });
            }
          } catch {
            // ignore
          }
        }
      }
    } catch (err: any) {
      console.error(err);
    }
  };

  useEffect(() => {
    loadData();
  }, [searchParams]);

  // When a vehicle is picked or plate changed
  const handleSelectVehicle = (plate: string) => {
    setSelectedPlate(plate);
    setError(null);

    const slot = occupiedSlots.find(
      (s) => s.license_plate?.toUpperCase() === plate.trim().toUpperCase()
    );

    if (slot) {
      setActiveSlot(slot);
      let fee = 250.0;
      if (slot.category === 'BIKE') fee = 100.0;
      else if (slot.category === 'EV') fee = 200.0;

      if (slot.estimated_fee && slot.estimated_fee > 0) {
        fee = slot.estimated_fee;
      }
      setAmount(fee.toFixed(2));
    } else {
      setActiveSlot(null);
      if (!amount || amount === '0.00') {
        setAmount('250.00');
      }
    }
  };

  const handlePayment = async (e: React.FormEvent) => {
    e.preventDefault();
    const parsedAmount = parseFloat(amount);
    if (isNaN(parsedAmount) || parsedAmount <= 0) {
      setError('Please enter a valid amount.');
      return;
    }

    try {
      setLoading(true);
      setError(null);

      const res = await api.processPayment({
        license_plate: selectedPlate.trim() || activeSlot?.license_plate || undefined,
        session_id: activeSlot?.session_id || 0,
        booking_id: activeSlot?.booking_id || undefined,
        amount: parsedAmount,
        payment_method: paymentMethod,
      });

      // Prepare official digital receipt
      setReceipt({
        payment_id: res.payment_id,
        license_plate: selectedPlate.trim() || activeSlot?.license_plate || 'DIRECT CHECKOUT',
        driver_name: activeSlot?.customer_name || 'Customer',
        phone: activeSlot?.customer_phone || '—',
        slot_number: activeSlot?.slot_number || 'Cashier Counter',
        category: activeSlot?.category || 'CAR',
        amount: parsedAmount,
        method: paymentMethod,
        timestamp: new Date().toLocaleTimeString('en-IN', {
          hour: '2-digit',
          minute: '2-digit',
          second: '2-digit',
          hour12: true,
        }),
      });

      // Reset selection and refresh data
      setSelectedPlate('');
      setActiveSlot(null);
      setAmount('250.00');
      loadData();
    } catch (err: any) {
      setError(err.message || 'Payment processing failed');
    } finally {
      setLoading(false);
    }
  };

  const handlePrint = () => {
    window.print();
  };

  return (
    <div className="space-y-8">
      {/* Page Header */}
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-2">
            <Receipt className="w-6 h-6 text-emerald-400" />
            Easy Billing &amp; Invoices
          </h1>
          <p className="text-sm text-slate-400 mt-1">
            Fast checkout: choose a parked vehicle, pick a payment method, and collect fees.
          </p>
        </div>
        <button
          onClick={loadData}
          className="flex items-center gap-2 px-3.5 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold rounded-xl border border-slate-700 transition cursor-pointer"
        >
          <RefreshCw className="w-3.5 h-3.5" />
          Refresh Status
        </button>
      </div>

      {error && (
        <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-xl text-rose-300 text-sm flex items-center gap-2">
          <span>⚠️</span>
          <span>{error}</span>
        </div>
      )}

      {/* Digital Receipt Modal / Card */}
      {receipt && (
        <div className="p-6 bg-gradient-to-br from-emerald-950/60 to-slate-900 border-2 border-emerald-500/80 rounded-2xl space-y-5 shadow-2xl animate-in fade-in zoom-in duration-200">
          <div className="flex items-center justify-between border-b border-emerald-500/30 pb-4">
            <div className="flex items-center gap-2 text-emerald-400 font-bold text-lg">
              <CheckCircle2 className="w-6 h-6" />
              Official Payment Receipt
            </div>
            <span className="text-xs font-mono px-3 py-1 bg-emerald-500/20 text-emerald-300 border border-emerald-500/40 rounded-full font-bold">
              RECEIPT #{receipt.payment_id}
            </span>
          </div>

          <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 text-sm">
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Vehicle Plate</span>
              <span className="font-mono font-bold text-white text-base">
                {receipt.license_plate}
              </span>
            </div>
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Driver Name</span>
              <span className="font-semibold text-slate-100 text-sm truncate block">
                {receipt.driver_name}
              </span>
            </div>
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Assigned Slot</span>
              <span className="font-bold text-indigo-400 text-base">
                {receipt.slot_number}
              </span>
            </div>
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Payment Mode</span>
              <span className="font-semibold text-emerald-300 text-sm">
                {receipt.method === 'UPI' ? '📱 UPI / QR' : receipt.method === 'CARD' ? '💳 Card' : '💵 Cash'}
              </span>
            </div>
          </div>

          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between p-4 bg-emerald-950/40 border border-emerald-600/40 rounded-xl gap-3">
            <div>
              <span className="text-xs text-emerald-300 block">Total Amount Paid</span>
              <span className="text-2xl font-black text-emerald-400">
                ₹{receipt.amount.toFixed(2)}
              </span>
            </div>
            <div className="text-right">
              <span className="text-xs text-slate-400 block">Paid At</span>
              <span className="text-xs font-mono text-slate-300">{receipt.timestamp}</span>
            </div>
          </div>

          <div className="flex items-center justify-end gap-3 pt-2">
            <button
              onClick={handlePrint}
              className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-semibold flex items-center gap-2 border border-slate-700 transition cursor-pointer"
            >
              <Printer className="w-4 h-4 text-slate-300" />
              Print Receipt
            </button>
            <button
              onClick={() => setReceipt(null)}
              className="px-5 py-2 bg-emerald-600 hover:bg-emerald-500 text-white rounded-xl text-xs font-bold transition shadow-lg shadow-emerald-600/30 cursor-pointer"
            >
              Next Customer / Done
            </button>
          </div>
        </div>
      )}

      {/* Main Billing Grid */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        {/* Simple Cashier Checkout Form */}
        <div className="lg:col-span-2 p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-6">
          <div className="border-b border-slate-800 pb-3 flex items-center justify-between">
            <div className="text-base font-semibold text-white flex items-center gap-2">
              <Sparkles className="w-5 h-5 text-indigo-400" />
              Quick Cashier Checkout
            </div>
            <span className="text-xs text-slate-400">Step-by-step easy billing</span>
          </div>

          <form onSubmit={handlePayment} className="space-y-6">
            {/* Step 1: Vehicle Selection */}
            <div className="space-y-3">
              <label className="block text-xs font-bold uppercase tracking-wider text-slate-300">
                1. Select Currently Parked or Exiting Vehicle
              </label>

              {occupiedSlots.length > 0 ? (
                <div className="grid grid-cols-1 sm:grid-cols-2 gap-2 max-h-48 overflow-y-auto pr-1">
                  {occupiedSlots.map((s) => {
                    const isSelected =
                      selectedPlate.toUpperCase() === s.license_plate?.toUpperCase();
                    return (
                      <button
                        type="button"
                        key={s.slot_id}
                        onClick={() => handleSelectVehicle(s.license_plate || '')}
                        className={`p-3 rounded-xl border text-left transition flex items-center justify-between cursor-pointer ${
                          isSelected
                            ? 'bg-indigo-950/80 border-indigo-500 shadow-md ring-1 ring-indigo-500'
                            : 'bg-slate-800/60 border-slate-700 hover:bg-slate-800 hover:border-slate-600'
                        }`}
                      >
                        <div className="flex items-center gap-3">
                          <div className="p-2 rounded-lg bg-slate-900 text-indigo-400">
                            {s.category === 'BIKE' ? (
                              <Bike className="w-4 h-4" />
                            ) : s.category === 'EV' ? (
                              <Zap className="w-4 h-4 text-emerald-400" />
                            ) : (
                              <Car className="w-4 h-4 text-indigo-400" />
                            )}
                          </div>
                          <div>
                            <div className="font-mono font-bold text-white text-sm">
                              {s.license_plate}
                            </div>
                            <div className="text-xs text-slate-400 flex items-center gap-1.5">
                              <span>Slot {s.slot_number}</span>
                              {s.customer_name && (
                                <>
                                  <span>•</span>
                                  <span className="truncate max-w-[100px]">
                                    {s.customer_name}
                                  </span>
                                </>
                              )}
                            </div>
                          </div>
                        </div>
                        <div className="text-right">
                          <span className="text-xs font-bold text-emerald-400 block">
                            ₹{(s.estimated_fee ?? (s.category === 'BIKE' ? 100 : s.category === 'EV' ? 200 : 250)).toFixed(0)}
                          </span>
                          <span className="text-[10px] text-slate-500">
                            {s.category}
                          </span>
                        </div>
                      </button>
                    );
                  })}
                </div>
              ) : (
                <div className="p-4 bg-slate-800/40 border border-slate-700/60 rounded-xl text-center text-xs text-slate-400">
                  {selectedPlate ? `Vehicle ${selectedPlate} selected from Gate Exit.` : 'No vehicles currently inside. You can type any plate below.'}
                </div>
              )}

              {/* Or manual plate input */}
              <div className="pt-2">
                <div className="relative">
                  <Search className="w-4 h-4 text-slate-400 absolute left-3 top-3" />
                  <input
                    type="text"
                    placeholder="Or type / search License Plate (e.g. UP-14-CC-9999)..."
                    value={selectedPlate}
                    onChange={(e) => handleSelectVehicle(e.target.value)}
                    className="w-full bg-slate-800 border border-slate-700 rounded-xl pl-9 pr-4 py-2.5 text-sm text-slate-100 font-mono font-semibold focus:outline-none focus:border-indigo-500"
                  />
                </div>
              </div>
            </div>

            {/* Auto-filled Vehicle & Driver Summary Banner */}
            {activeSlot && (
              <div className="p-4 bg-indigo-950/30 border border-indigo-800/60 rounded-xl grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
                <div>
                  <span className="text-slate-400 block mb-0.5">Driver Name</span>
                  <div className="font-semibold text-white flex items-center gap-1">
                    <User className="w-3.5 h-3.5 text-indigo-400" />
                    <span>{activeSlot.customer_name || 'Customer'}</span>
                  </div>
                </div>
                <div>
                  <span className="text-slate-400 block mb-0.5">Phone Number</span>
                  <div className="font-mono text-slate-200 flex items-center gap-1">
                    <Phone className="w-3.5 h-3.5 text-indigo-400" />
                    <span>{activeSlot.customer_phone || '—'}</span>
                  </div>
                </div>
                <div>
                  <span className="text-slate-400 block mb-0.5">Category Rate</span>
                  <span className="font-bold text-amber-300">
                    {activeSlot.category === 'BIKE'
                      ? '₹100 / hr (Bike)'
                      : activeSlot.category === 'EV'
                      ? '₹200 / hr (EV)'
                      : '₹250 / hr (Car)'}
                  </span>
                </div>
                <div>
                  <span className="text-slate-400 block mb-0.5">Assigned / Vacated Slot</span>
                  <div className="font-semibold text-slate-200 flex items-center gap-1">
                    <Clock className="w-3.5 h-3.5 text-indigo-400" />
                    <span>Slot {activeSlot.slot_number}</span>
                  </div>
                </div>
              </div>
            )}

            {/* Step 2: Payment Method Pills */}
            <div className="space-y-3">
              <label className="block text-xs font-bold uppercase tracking-wider text-slate-300">
                2. Choose Payment Mode
              </label>
              <div className="grid grid-cols-3 gap-3">
                <button
                  type="button"
                  onClick={() => setPaymentMethod('CASH')}
                  className={`py-3 px-4 rounded-xl border text-sm font-semibold flex items-center justify-center gap-2 transition cursor-pointer ${
                    paymentMethod === 'CASH'
                      ? 'bg-emerald-600 text-white border-emerald-500 shadow-lg shadow-emerald-600/30 ring-2 ring-emerald-400'
                      : 'bg-slate-800 text-slate-300 border-slate-700 hover:bg-slate-700 hover:text-white'
                  }`}
                >
                  <Banknote className="w-4 h-4" />
                  <span>Cash</span>
                </button>

                <button
                  type="button"
                  onClick={() => setPaymentMethod('UPI')}
                  className={`py-3 px-4 rounded-xl border text-sm font-semibold flex items-center justify-center gap-2 transition cursor-pointer ${
                    paymentMethod === 'UPI'
                      ? 'bg-indigo-600 text-white border-indigo-500 shadow-lg shadow-indigo-600/30 ring-2 ring-indigo-400'
                      : 'bg-slate-800 text-slate-300 border-slate-700 hover:bg-slate-700 hover:text-white'
                  }`}
                >
                  <QrCode className="w-4 h-4" />
                  <span>UPI / QR</span>
                </button>

                <button
                  type="button"
                  onClick={() => setPaymentMethod('CARD')}
                  className={`py-3 px-4 rounded-xl border text-sm font-semibold flex items-center justify-center gap-2 transition cursor-pointer ${
                    paymentMethod === 'CARD'
                      ? 'bg-purple-600 text-white border-purple-500 shadow-lg shadow-purple-600/30 ring-2 ring-purple-400'
                      : 'bg-slate-800 text-slate-300 border-slate-700 hover:bg-slate-700 hover:text-white'
                  }`}
                >
                  <CreditCard className="w-4 h-4" />
                  <span>Card</span>
                </button>
              </div>
            </div>

            {/* Step 3: Total Fee & Collect Button */}
            <div className="pt-2 space-y-4">
              <div>
                <label className="block text-xs font-bold uppercase tracking-wider text-slate-300 mb-1.5">
                  3. Total Amount Due (₹)
                </label>
                <div className="relative">
                  <span className="absolute left-4 top-3 text-lg font-bold text-slate-400">
                    ₹
                  </span>
                  <input
                    type="number"
                    step="0.01"
                    required
                    value={amount}
                    onChange={(e) => setAmount(e.target.value)}
                    className="w-full bg-slate-800 border-2 border-slate-700 focus:border-emerald-500 rounded-xl pl-9 pr-4 py-3 text-xl font-bold font-mono text-emerald-400 focus:outline-none transition"
                  />
                </div>
              </div>

              <button
                type="submit"
                disabled={loading}
                className="w-full py-4 bg-emerald-600 hover:bg-emerald-500 active:scale-[0.99] text-white rounded-xl text-base font-bold shadow-xl shadow-emerald-600/30 transition flex items-center justify-center gap-2 cursor-pointer"
              >
                {loading ? (
                  <>
                    <RefreshCw className="w-5 h-5 animate-spin" />
                    <span>Processing Payment...</span>
                  </>
                ) : (
                  <>
                    <span>Collect ₹{parseFloat(amount || '0').toFixed(2)} &amp; Issue Receipt</span>
                    <ArrowRight className="w-5 h-5" />
                  </>
                )}
              </button>
            </div>
          </form>
        </div>

        {/* Right Column: Financial Overview Cards */}
        <div className="space-y-6">
          <div className="p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-4">
            <h2 className="text-base font-semibold text-white">Facility Revenue</h2>

            <div className="space-y-4">
              <div className="p-4 bg-slate-800/60 rounded-xl border border-slate-800">
                <span className="text-slate-400 text-xs block">Cumulative Revenue Collected</span>
                <span className="text-3xl font-black text-emerald-400 mt-1 block">
                  ₹{summary?.total_revenue?.toFixed(2) ?? '0.00'}
                </span>
                <span className="text-[10px] text-slate-500 mt-1.5 block">
                  Aggregated from verified SQLite3 payments
                </span>
              </div>

              <div className="p-4 bg-slate-800/60 rounded-xl border border-slate-800">
                <span className="text-slate-400 text-xs block">Current Occupancy</span>
                <span className="text-2xl font-bold text-amber-400 mt-1 block">
                  {summary?.occupancy_rate?.toFixed(1) ?? '0.0'}%
                </span>
                <span className="text-[10px] text-slate-500 mt-1.5 block">
                  {summary?.occupied_slots ?? 0} active parking spaces generating revenue
                </span>
              </div>

              <div className="p-4 bg-slate-800/60 rounded-xl border border-slate-800">
                <span className="text-slate-400 text-xs block">Standard Facility Rates</span>
                <div className="mt-2 space-y-1.5 text-xs text-slate-300">
                  <div className="flex justify-between items-center">
                    <span className="flex items-center gap-1.5 text-indigo-300">
                      <Car className="w-3.5 h-3.5" /> Car
                    </span>
                    <span className="font-bold text-white font-mono">₹250 / hr</span>
                  </div>
                  <div className="flex justify-between items-center">
                    <span className="flex items-center gap-1.5 text-sky-300">
                      <Bike className="w-3.5 h-3.5" /> Bike
                    </span>
                    <span className="font-bold text-white font-mono">₹100 / hr</span>
                  </div>
                  <div className="flex justify-between items-center">
                    <span className="flex items-center gap-1.5 text-emerald-300">
                      <Zap className="w-3.5 h-3.5" /> EV Station
                    </span>
                    <span className="font-bold text-white font-mono">₹200 / hr</span>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default function BillingPage() {
  return (
    <Suspense fallback={<div className="p-8 text-slate-400">Loading Billing...</div>}>
      <BillingContent />
    </Suspense>
  );
}
