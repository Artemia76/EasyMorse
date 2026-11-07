// EasyMorse server administration - GPLv3 - https://github.com/Artemia76/EasyMorse
"use strict";

const french = {
  title: "Administration",
  back: "Retour à la page du serveur",
  login: "Connexion administrateur",
  callsign: "Indicatif",
  password: "Mot de passe",
  enter: "Se connecter",
  accounts: "Comptes",
  logout: "Se déconnecter",
  filter: "Filtrer par indicatif",
  notAdmin: "Ce compte n'est pas administrateur.",
  wrong: "Indicatif ou mot de passe incorrect.",
  unreachable: "Serveur injoignable.",
  whoami: (c) => `Connecté en tant que ${c}`,
  summary: (n, o) => `${n} compte(s), ${o} connecté(s)`,
  online: "en ligne",
  inRoom: (r) => `en ligne, salon ${r}`,
  offline: "hors ligne",
  admin: "administrateur",
  banned: "suspendu",
  you: "vous",
  created: "créé le",
  lastLogin: "dernière connexion",
  never: "jamais",
  kick: "Déconnecter",
  ban: "Suspendre",
  unban: "Réactiver",
  grant: "Nommer administrateur",
  revoke: "Retirer administrateur",
  setPassword: "Nouveau mot de passe",
  remove: "Supprimer",
  askReason: (c) => `Raison de la suspension de ${c} (montrée à l'utilisateur) :`,
  askPassword: (c) => `Nouveau mot de passe pour ${c} (8 caractères ou plus) :`,
  confirmDelete: (c) => `Supprimer définitivement le compte ${c} ?`,
  confirmKick: (c) => `Déconnecter ${c} ? Il devra saisir à nouveau son mot de passe.`,
  done: (a, c) => `${a} : ${c}`,
  none: "Aucun compte.",
  quote: (t) => `« ${t} »`,
};
const english = {
  notAdmin: "This account is not an administrator.",
  wrong: "Wrong callsign or password.",
  unreachable: "Server unreachable.",
  whoami: (c) => `Logged in as ${c}`,
  summary: (n, o) => `${n} account(s), ${o} online`,
  online: "online",
  inRoom: (r) => `online, room ${r}`,
  offline: "offline",
  admin: "administrator",
  banned: "suspended",
  you: "you",
  created: "created",
  lastLogin: "last login",
  never: "never",
  kick: "Disconnect",
  ban: "Suspend",
  unban: "Reactivate",
  grant: "Make administrator",
  revoke: "Remove administrator",
  setPassword: "New password",
  remove: "Delete",
  askReason: (c) => `Reason for suspending ${c} (shown to the user):`,
  askPassword: (c) => `New password for ${c} (8 characters or more):`,
  confirmDelete: (c) => `Delete the account ${c} for good?`,
  confirmKick: (c) => `Disconnect ${c}? They will have to type their password again.`,
  done: (a, c) => `${a}: ${c}`,
  none: "No account.",
  quote: (t) => `“${t}”`,
};
const text = navigator.language.startsWith("fr") ? french : english;
const locale = navigator.language;

if (text === french) {
  document.documentElement.lang = "fr";
  for (const element of document.querySelectorAll("[data-i18n]")) {
    const value = french[element.dataset.i18n];
    if (typeof value === "string") element.textContent = value;
  }
  for (const element of document.querySelectorAll("[data-i18n-placeholder]"))
    element.placeholder = french[element.dataset.i18nPlaceholder];
}

// The token lives only as long as the tab
let token = sessionStorage.getItem("adminToken") || "";
let me = "";
let users = [];
let timer = 0;

async function api(method, path, body) {
  const response = await fetch(path, {
    method,
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${token}` },
    body: body ? JSON.stringify(body) : undefined,
  });
  const answer = response.status === 204 ? {} : await response.json().catch(() => ({}));
  return { ok: response.ok, status: response.status, answer };
}

function show(admin) {
  document.getElementById("login-section").hidden = admin;
  document.getElementById("admin-section").hidden = !admin;
  clearInterval(timer);
  if (admin) {
    document.getElementById("whoami").textContent = text.whoami(me);
    refresh();
    timer = setInterval(refresh, 10000);
  }
}

function date(seconds) {
  return seconds ? new Date(seconds * 1000).toLocaleString(locale, { dateStyle: "short", timeStyle: "short" })
                 : text.never;
}

function badge(label, kind) {
  const element = document.createElement("span");
  element.className = `badge ${kind}`;
  element.textContent = label;
  return element;
}

function button(label, onClick, danger) {
  const element = document.createElement("button");
  element.type = "button";
  element.className = danger ? "small danger" : "small";
  element.textContent = label;
  element.addEventListener("click", onClick);
  return element;
}

function render() {
  const filter = document.getElementById("filter").value.trim().toUpperCase();
  const list = document.getElementById("users");
  const shown = users.filter((user) => user.callsign.includes(filter));
  if (shown.length === 0) {
    const empty = document.createElement("p");
    empty.className = "hint";
    empty.textContent = text.none;
    list.replaceChildren(empty);
    return;
  }
  list.replaceChildren(...shown.map((user) => {
    const card = document.createElement("div");
    card.className = user.banned ? "user banned" : "user";

    const title = document.createElement("div");
    title.className = "user-title";
    const name = document.createElement("strong");
    name.textContent = user.callsign;
    title.append(name);
    if (user.callsign === me) title.append(badge(text.you, "muted"));
    if (user.admin) title.append(badge(text.admin, "accent"));
    if (user.banned) title.append(badge(text.banned, "error"));
    title.append(user.online ? badge(user.room ? text.inRoom(user.room) : text.online, "ok")
                             : badge(text.offline, "muted"));

    const details = document.createElement("div");
    details.className = "hint";
    details.textContent = `${text.created} ${date(user.created)} · ${text.lastLogin} ${date(user.lastLogin)}`;
    if (user.banned && user.reason) details.textContent += ` · ${text.quote(user.reason)}`;

    const actions = document.createElement("div");
    actions.className = "actions";
    const self = user.callsign === me;
    if (user.online && !self)
      actions.append(button(text.kick, () => confirm(text.confirmKick(user.callsign)) && act("kick", user, text.kick)));
    if (!self) {
      actions.append(user.banned
        ? button(text.unban, () => act("unban", user, text.unban))
        : button(text.ban, () => {
            const reason = prompt(text.askReason(user.callsign), "");
            if (reason !== null) act("ban", user, text.ban, { reason });
          }));
    }
    if (!user.admin) actions.append(button(text.grant, () => act("grant", user, text.grant)));
    else if (!self) actions.append(button(text.revoke, () => act("revoke", user, text.revoke)));
    actions.append(button(text.setPassword, () => {
      const password = prompt(text.askPassword(user.callsign), "");
      if (password) act("password", user, text.setPassword, { password });
    }));
    if (!self)
      actions.append(button(text.remove, () => confirm(text.confirmDelete(user.callsign)) && act("delete", user, text.remove), true));

    card.append(title, details, actions);
    return card;
  }));
}

function report(message, ok) {
  const result = document.getElementById("action-result");
  result.className = ok ? "ok" : "error";
  result.textContent = message;
}

async function act(action, user, label, extra = {}) {
  try {
    const reply = await api("POST", "/api/v1/admin/users", { action, callsign: user.callsign, ...extra });
    if (reply.status === 401 || reply.status === 403) return show(false);
    report(reply.ok ? text.done(label, user.callsign) : reply.answer.message || reply.status, reply.ok);
    refresh();
  } catch {
    report(text.unreachable, false);
  }
}

async function refresh() {
  try {
    const reply = await api("GET", "/api/v1/admin/users");
    if (!reply.ok) {
      token = "";
      sessionStorage.removeItem("adminToken");
      return show(false);
    }
    users = reply.answer.users;
    document.getElementById("summary").textContent = text.summary(users.length, reply.answer.online);
    render();
  } catch {
    report(text.unreachable, false);
  }
}

document.getElementById("filter").addEventListener("input", render);

document.getElementById("login").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = event.target;
  const result = document.getElementById("login-result");
  result.textContent = "";
  try {
    const response = await fetch("/api/v1/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ callsign: form.callsign.value, password: form.password.value }),
    });
    const answer = await response.json();
    form.password.value = "";
    if (!response.ok) {
      result.textContent = response.status === 401 ? text.wrong : answer.message || text.wrong;
      return;
    }
    token = answer.token;
    if (!answer.admin) {
      result.textContent = text.notAdmin;
      await api("POST", "/api/v1/logout");
      token = "";
      return;
    }
    me = answer.callsign;
    sessionStorage.setItem("adminToken", token);
    show(true);
  } catch {
    result.textContent = text.unreachable;
  }
});

document.getElementById("logout").addEventListener("click", async () => {
  try { await api("POST", "/api/v1/logout"); } catch { /* the token is forgotten anyway */ }
  token = "";
  sessionStorage.removeItem("adminToken");
  show(false);
});

// Back on the page with a token still valid
(async () => {
  if (!token) return show(false);
  try {
    const reply = await api("GET", "/api/v1/me");
    if (reply.ok && reply.answer.admin) {
      me = reply.answer.callsign;
      return show(true);
    }
  } catch { /* shown as logged out */ }
  token = "";
  sessionStorage.removeItem("adminToken");
  show(false);
})();
