// @vitest-environment jsdom
import { describe, it, expect, vi } from 'vitest';
import { createDropdown } from './controls';

const OPTS = [
  { value: '0', label: 'Novice' },
  { value: '1', label: 'Beginner' },
  { value: '2', label: 'Advanced' },
];

const key = (el: Element, k: string) =>
  el.dispatchEvent(new KeyboardEvent('keydown', { key: k, bubbles: true }));

describe('createDropdown', () => {
  it('starts closed, on the initial value', () => {
    const d = createDropdown(OPTS, '1');
    const btn = d.el.querySelector('[role=combobox]')!;
    expect(btn.getAttribute('aria-expanded')).toBe('false');
    expect(d.value).toBe('1');
    expect(btn.textContent).toContain('Beginner');
  });

  it('opens on Enter and closes on Escape, returning focus', () => {
    const d = createDropdown(OPTS);
    document.body.appendChild(d.el);
    const btn = d.el.querySelector('[role=combobox]') as HTMLElement;
    btn.focus();
    key(btn, 'Enter');
    expect(btn.getAttribute('aria-expanded')).toBe('true');
    key(d.el.querySelector('[role=listbox]')!, 'Escape');
    expect(btn.getAttribute('aria-expanded')).toBe('false');
    expect(document.activeElement).toBe(btn);
  });

  it('ArrowDown then Enter selects the next option and emits change', () => {
    const d = createDropdown(OPTS, '0');
    document.body.appendChild(d.el);
    const onChange = vi.fn();
    d.el.addEventListener('change', onChange);
    const btn = d.el.querySelector('[role=combobox]') as HTMLElement;
    key(btn, 'Enter');
    const list = d.el.querySelector('[role=listbox]')!;
    key(list, 'ArrowDown');
    key(list, 'Enter');
    expect(d.value).toBe('1');
    expect(onChange).toHaveBeenCalledTimes(1);
  });

  it('End jumps to the last option', () => {
    const d = createDropdown(OPTS, '0');
    document.body.appendChild(d.el);
    const btn = d.el.querySelector('[role=combobox]') as HTMLElement;
    key(btn, 'Enter');
    const list = d.el.querySelector('[role=listbox]')!;
    key(list, 'End');
    key(list, 'Enter');
    expect(d.value).toBe('2');
  });

  it('marks the selected option with aria-selected', () => {
    const d = createDropdown(OPTS, '2');
    const selected = d.el.querySelectorAll('[role=option][aria-selected=true]');
    expect(selected).toHaveLength(1);
    expect(selected[0].textContent).toContain('Advanced');
  });
});
