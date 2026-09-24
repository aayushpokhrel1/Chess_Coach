export interface DropdownOption {
  value: string;
  label: string;
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
