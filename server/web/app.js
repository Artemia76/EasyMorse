// EasyMorse server page - GPLv3 - https://github.com/Artemia76/EasyMorse
"use strict";

const french = {
  tagline: "Apprenez le morse à l'oreille et au manipulateur, et pratiquez avec d'autres opérateurs.",
  how: "Comment ça marche",
  step1: "Installez EasyMorse sur votre ordinateur.",
  step2: "Créez votre compte ci-dessous, ou depuis l'onglet Réseau de l'application.",
  step3: "Dans l'application, saisissez l'adresse de ce serveur et rejoignez un salon : vous êtes relié aux opérateurs présents.",
  download: "Télécharger EasyMorse",
  address: "Adresse du serveur pour l'application :",
  rooms: "Salons",
  loading: "Chargement…",
  account: "Créer un compte",
  callsign: "Indicatif",
  password: "Mot de passe (8 caractères ou plus)",
  create: "Créer",
  free: "Logiciel libre (GPLv3).",
  source: "Code source",
  members: (n) => n === 0 ? "vide" : n === 1 ? "1 opérateur" : `${n} opérateurs`,
  online: (n) => `${n} connecté(s) au serveur`,
  created: (c) => `Compte ${c} créé. Connectez-vous depuis l'application.`,
  unreachable: "Serveur injoignable.",
};
const english = {
  members: (n) => n === 0 ? "empty" : n === 1 ? "1 operator" : `${n} operators`,
  online: (n) => `${n} connected to the server`,
  created: (c) => `Account ${c} created. Log in from the application.`,
  unreachable: "Server unreachable.",
};
const text = navigator.language.startsWith("fr") ? french : english;

if (text === french) {
  document.documentElement.lang = "fr";
  for (const element of document.querySelectorAll("[data-i18n]")) {
    const value = french[element.dataset.i18n];
    if (typeof value === "string") element.textContent = value;
  }
}
document.getElementById("address").textContent = location.host;

async function refreshRooms() {
  const list = document.getElementById("rooms");
  try {
    const [rooms, status] = await Promise.all([
      fetch("/api/v1/rooms").then((r) => r.json()),
      fetch("/api/v1/status").then((r) => r.json()),
    ]);
    list.replaceChildren(...rooms.rooms.map((room) => {
      const item = document.createElement("li");
      const name = document.createElement("strong");
      name.textContent = room.name;
      item.append(name, " — ", text.members(room.members));
      return item;
    }));
    document.getElementById("status").textContent = text.online(status.online);
  } catch {
    list.replaceChildren(text.unreachable);
  }
}

document.getElementById("register").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = event.target;
  const result = document.getElementById("result");
  const body = { callsign: form.callsign.value, password: form.password.value };
  try {
    const response = await fetch("/api/v1/register", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    });
    const answer = await response.json();
    result.className = response.ok ? "ok" : "error";
    // The token is not kept: the application logs in by itself
    result.textContent = response.ok ? text.created(answer.callsign) : answer.message;
    if (response.ok) form.reset();
  } catch {
    result.className = "error";
    result.textContent = text.unreachable;
  }
});

refreshRooms();
setInterval(refreshRooms, 10000);
