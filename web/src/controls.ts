export interface DropdownOption {
  value: string;
  label: string;
}
export interface Segment {
  value: string;
  label: string;
}
export interface Level {
  name: string;
  elo: number | null;
  blurb: string;
  depth: number;
  random?: number;
}
export interface Dropdown {
  el: HTMLElement;
  get value(): string;
  set value(v: string);
  destroy(): void;
}

// A native <select> cannot be styled to belong to the room, so the dropdown is
// rebuilt from a button and a listbox. The button carries the state; the listbox
// carries the keys while it is open, which is where the focus stays so the
// browser does not scroll the page on every arrow press.
export function createDropdown(options: DropdownOption[], initial?: string): Dropdown {
  const el = document.createElement('div');
  el.className = 'dropdown';

  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'dropdown-button';
  button.setAttribute('role', 'combobox');
  button.setAttribute('aria-haspopup', 'listbox');
  button.setAttribute('aria-expanded', 'false');

  const list = document.createElement('ul');
  list.className = 'dropdown-list';
  list.setAttribute('role', 'listbox');
  list.tabIndex = -1;
  list.hidden = true;

  const listId = `dropdown-list-${Math.random().toString(36).slice(2, 9)}`;
  list.id = listId;
  button.setAttribute('aria-controls', listId);

  const items: HTMLLIElement[] = options.map((option) => {
    const li = document.createElement('li');
    li.className = 'dropdown-option';
    li.setAttribute('role', 'option');
    li.setAttribute('aria-selected', 'false');
    li.dataset.value = option.value;
    li.textContent = option.label;
    li.addEventListener('click', () => {
      select(option.value);
      close();
      button.focus();
    });
    list.appendChild(li);
    return li;
  });

  let selected = initial ?? options[0]?.value ?? '';
  let active = Math.max(
    0,
    options.findIndex((o) => o.value === selected),
  );
  let open = false;

  function render() {
    const current = options.find((o) => o.value === selected);
    button.textContent = current ? current.label : '';
    items.forEach((li, i) => {
      const isSelected = li.dataset.value === selected;
      li.setAttribute('aria-selected', isSelected ? 'true' : 'false');
      li.classList.toggle('ds-active', open && i === active);
      li.classList.toggle('is-selected', isSelected);
    });
  }

  function openList() {
    if (open) return;
    open = true;
    list.hidden = false;
    button.setAttribute('aria-expanded', 'true');
    el.classList.add('is-open');
    render();
    list.focus();
  }

  function close() {
    if (!open) return;
    open = false;
    list.hidden = true;
    button.setAttribute('aria-expanded', 'false');
    el.classList.remove('is-open');
    render();
  }

  function select(value: string) {
    if (value === selected) return;
    selected = value;
    active = Math.max(
      0,
      options.findIndex((o) => o.value === value),
    );
    render();
    el.dispatchEvent(new Event('change', { bubbles: true }));
  }

  function move(delta: number) {
    if (options.length === 0) return;
    active = (active + delta + options.length) % options.length;
    render();
  }

  function onButtonKeydown(e: KeyboardEvent) {
    if (e.key === 'Enter' || e.key === ' ' || e.key === 'ArrowDown' || e.key === 'ArrowUp') {
      e.preventDefault();
      openList();
    }
  }

  function onListKeydown(e: KeyboardEvent) {
    switch (e.key) {
      case 'ArrowDown':
        e.preventDefault();
        move(1);
        break;
      case 'ArrowUp':
        e.preventDefault();
        move(-1);
        break;
      case 'Home':
        e.preventDefault();
        active = 0;
        render();
        break;
      case 'End':
        e.preventDefault();
        active = options.length - 1;
        render();
        break;
      case 'Enter':
      case ' ':
        e.preventDefault();
        select(options[active]?.value ?? selected);
        close();
        button.focus();
        break;
      case 'Escape':
        e.preventDefault();
        close();
        button.focus();
        break;
      case 'Tab':
        close();
        break;
      default:
        break;
    }
  }

  function onDocumentClick(e: MouseEvent) {
    if (!el.contains(e.target as Node)) close();
  }

  function onScroll() {
    close();
  }

  button.addEventListener('keydown', onButtonKeydown);
  list.addEventListener('keydown', onListKeydown);
  document.addEventListener('click', onDocumentClick);
  document.addEventListener('scroll', onScroll, true);

  el.appendChild(button);
  el.appendChild(list);
  render();

  return {
    el,
    get value() {
      return selected;
    },
    set value(v: string) {
      selected = v;
      active = Math.max(
        0,
        options.findIndex((o) => o.value === v),
      );
      render();
    },
    destroy() {
      button.removeEventListener('keydown', onButtonKeydown);
      list.removeEventListener('keydown', onListKeydown);
      document.removeEventListener('click', onDocumentClick);
      document.removeEventListener('scroll', onScroll, true);
      el.remove();
    },
  };
}

// A pill row of mutually exclusive options. The buttons are the state; the
// indicator is a separate element that slides under the selected one with a
// transform, so the browser never lays out on every key press.
export function createSegmented(segments: Segment[], initial?: string): Dropdown {
  const el = document.createElement('div');
  el.className = 'segmented';
  el.setAttribute('role', 'radiogroup');

  const indicator = document.createElement('span');
  indicator.className = 'segmented-indicator';
  indicator.setAttribute('aria-hidden', 'true');
  el.appendChild(indicator);

  const buttons: HTMLButtonElement[] = segments.map((segment) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'segmented-option';
    button.setAttribute('role', 'radio');
    button.setAttribute('aria-checked', 'false');
    button.dataset.value = segment.value;
    button.textContent = segment.label;
    button.addEventListener('click', () => {
      select(segment.value);
      button.focus();
    });
    el.appendChild(button);
    return button;
  });

  let selected = initial ?? segments[0]?.value ?? '';
  let active = Math.max(
    0,
    segments.findIndex((s) => s.value === selected),
  );

  function render() {
    buttons.forEach((button, i) => {
      const isSelected = button.dataset.value === selected;
      button.setAttribute('aria-checked', isSelected ? 'true' : 'false');
      button.classList.toggle('is-selected', isSelected);
      button.tabIndex = i === active ? 0 : -1;
    });
    const current = buttons[active];
    if (current) {
      indicator.style.transform = `translateX(${current.offsetLeft}px)`;
      indicator.style.width = `${current.offsetWidth}px`;
    }
  }

  function select(value: string) {
    if (value === selected) return;
    selected = value;
    active = Math.max(
      0,
      segments.findIndex((s) => s.value === value),
    );
    render();
    el.dispatchEvent(new Event('change', { bubbles: true }));
  }

  function move(delta: number) {
    if (segments.length === 0) return;
    active = (active + delta + segments.length) % segments.length;
    render();
    buttons[active]?.focus();
  }

  function onKeydown(e: KeyboardEvent) {
    switch (e.key) {
      case 'ArrowRight':
      case 'ArrowDown':
        e.preventDefault();
        move(1);
        break;
      case 'ArrowLeft':
      case 'ArrowUp':
        e.preventDefault();
        move(-1);
        break;
      case 'Home':
        e.preventDefault();
        active = 0;
        render();
        buttons[0]?.focus();
        break;
      case 'End':
        e.preventDefault();
        active = segments.length - 1;
        render();
        buttons[active]?.focus();
        break;
      case 'Enter':
      case ' ':
        e.preventDefault();
        select(segments[active]?.value ?? selected);
        break;
      default:
        break;
    }
  }

  el.addEventListener('keydown', onKeydown);
  render();

  return {
    el,
    get value() {
      return selected;
    },
    set value(v: string) {
      selected = v;
      active = Math.max(
        0,
        segments.findIndex((s) => s.value === v),
      );
      render();
    },
    destroy() {
      el.removeEventListener('keydown', onKeydown);
      el.remove();
    },
  };
}

// The strength picker: one card per level, the measured Elo as the numeral and
// the level's character as the copy. Novice has no rating (it is below the
// engine's floor), so it reads as unrated rather than a fabricated number.
export function createLevelPicker(levels: Level[], initial?: number): Dropdown {
  const el = document.createElement('div');
  el.className = 'level-picker';
  el.setAttribute('role', 'radiogroup');
  el.setAttribute('aria-label', 'Engine strength');

  const cards: HTMLButtonElement[] = levels.map((level, i) => {
    const card = document.createElement('button');
    card.type = 'button';
    card.className = 'level-card';
    card.setAttribute('role', 'radio');
    card.setAttribute('aria-checked', 'false');
    card.dataset.value = String(i);

    const elo = document.createElement('span');
    elo.className = 'level-elo';
    elo.textContent = level.elo === null ? 'unrated' : String(level.elo);

    const name = document.createElement('span');
    name.className = 'level-name';
    name.textContent = level.name;

    const blurb = document.createElement('span');
    blurb.className = 'level-blurb';
    blurb.textContent = level.blurb;

    card.appendChild(elo);
    card.appendChild(name);
    card.appendChild(blurb);
    card.addEventListener('click', () => {
      select(i);
      card.focus();
    });
    el.appendChild(card);
    return card;
  });

  let selected = initial ?? 0;
  let active = Math.max(0, Math.min(selected, levels.length - 1));

  function render() {
    cards.forEach((card, i) => {
      const isSelected = i === selected;
      card.setAttribute('aria-checked', isSelected ? 'true' : 'false');
      card.classList.toggle('is-selected', isSelected);
      card.tabIndex = i === active ? 0 : -1;
    });
  }

  function select(index: number) {
    if (index === selected) return;
    selected = index;
    active = index;
    render();
    el.dispatchEvent(new Event('change', { bubbles: true }));
  }

  function move(delta: number) {
    if (levels.length === 0) return;
    active = (active + delta + levels.length) % levels.length;
    render();
    cards[active]?.focus();
  }

  function onKeydown(e: KeyboardEvent) {
    switch (e.key) {
      case 'ArrowDown':
      case 'ArrowRight':
        e.preventDefault();
        move(1);
        break;
      case 'ArrowUp':
      case 'ArrowLeft':
        e.preventDefault();
        move(-1);
        break;
      case 'Home':
        e.preventDefault();
        active = 0;
        render();
        cards[0]?.focus();
        break;
      case 'End':
        e.preventDefault();
        active = levels.length - 1;
        render();
        cards[active]?.focus();
        break;
      case 'Enter':
      case ' ':
        e.preventDefault();
        select(active);
        break;
      default:
        break;
    }
  }

  el.addEventListener('keydown', onKeydown);
  render();

  return {
    el,
    get value() {
      return String(selected);
    },
    set value(v: string) {
      const index = Number(v);
      selected = Number.isFinite(index) ? Math.max(0, Math.min(index, levels.length - 1)) : 0;
      active = selected;
      render();
    },
    destroy() {
      el.removeEventListener('keydown', onKeydown);
      el.remove();
    },
  };
}
