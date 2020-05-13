module Styles = {
  open Css;
  let root =
    style([display(flexBox), flexDirection(column), minHeight(vh(100.))]);
  let body = style([flexGrow(1.)]);
  let tagline =
    style([
      textAlign(center),
      marginBottom(px(32)),
      media("(max-width: 500px)", [marginBottom(px(32))]),
    ]);
  let tooltip =
    style([
      backgroundColor(Colors.darkLayerBackground),
      borderRadius(px(4)),
      color(Colors.white),
      fontSize(px(14)),
      padding3(~top=px(5), ~bottom=px(3), ~h=px(10)),
      position(relative),
      whiteSpace(`preLine),
      Colors.darkLayerShadow,
    ]);
};

[@bs.val] [@bs.scope "window"]
external gtag: option((. string, string, {. "page_path": string}) => unit) =
  "gtag";

module TooltipConfigContextProvider = {
  type tooltipModifiers =
    array({
      .
      "name": string,
      "options": {. "offset": array(int)},
    });
  let makeProps = (~value, ~children, ()) => {
    "value": value,
    "children": children,
  };
  let make =
    React.Context.provider(
      ReactAtmosphere.Tooltip.configContext:
                                             React.Context.t(
                                               ReactAtmosphere.Tooltip.configContext(
                                                 tooltipModifiers,
                                               ),
                                             ),
    );
};

let tooltipConfig:
  ReactAtmosphere.Tooltip.configContext(
    TooltipConfigContextProvider.tooltipModifiers,
  ) = {
  renderTooltip: (tooltip, _) =>
    <div className=Styles.tooltip> tooltip </div>,
  options:
    Some({
      placement: Some("top"),
      modifiers:
        Some([|{
                 "name": "offset",
                 "options": {
                   "offset": [|0, 2|],
                 },
               }|]),
    }),
};

[@react.component]
let make = () => {
  let url = ReasonReactRouter.useUrl();
  let (showLogin, setShowLogin) = React.useState(() => false);
  let (showSettings, setShowSettings) = React.useState(() => false);
  let itemDetails = {
    let result = url.hash |> Js.Re.exec_([%bs.re "/i(-?\d+)(:(\d+))?/g"]);
    switch (result) {
    | Some(match) =>
      let captures = Js.Re.captures(match);
      let itemId = captures[1]->Js.Nullable.toOption->Belt.Option.getExn;
      Item.itemMap
      ->Js.Dict.get(itemId)
      ->Belt.Option.map(item =>
          (
            item,
            captures[3]->Js.Nullable.toOption->Belt.Option.map(int_of_string),
          )
        );
    | None => None
    };
  };

  let pathString =
    "/" ++ Js.Array.joinWith("/", Belt.List.toArray(url.path));
  React.useEffect0(() => {
    Analytics.Amplitude.logEventWithProperties(
      ~eventName="Session Started",
      ~eventProperties={
        "path":
          pathString
          ++ (
            switch (url.search) {
            | "" => ""
            | search => "?" ++ search
            }
          )
          ++ (
            switch (url.hash) {
            | "" => ""
            | hash => "#" ++ hash
            }
          ),
      },
    );
    None;
  });
  React.useEffect1(
    () => {
      switch (gtag) {
      | Some(gtag) =>
        gtag(. "config", Constants.gtagId, {"page_path": pathString})
      | None => ()
      };

      None;
    },
    [|pathString|],
  );
  let (_, forceUpdate) = React.useState(() => 1);

  let language = SettingsStore.useLanguage();
  let (isLanguageLoaded, setIsLanguageLoaded) =
    React.useState(() => language === `English);
  if (language === `English && !isLanguageLoaded) {
    setIsLanguageLoaded(_ => true);
  };
  React.useEffect1(
    () => {
      if (language !== `English) {
        Item.loadTranslation(
          SettingsStore.languageToJs(language),
          json => {
            setIsLanguageLoaded(_ => true);
            Item.setTranslations(json);
            forceUpdate(x => x + 1);
          },
        );
      } else {
        Item.clearTranslations();
        forceUpdate(x => x + 1);
      };
      Analytics.Amplitude.setLanguage(
        ~language=SettingsStore.languageToJs(language),
      );
      None;
    },
    [|language|],
  );

  let isInitialLoadRef = React.useRef(true);
  React.useEffect0(() => {
    React.Ref.setCurrent(isInitialLoadRef, false);
    Item.loadVariants(json => {
      Item.setVariantNames(json);
      forceUpdate(x => x + 1);
    });
    None;
  });

  <div className=Styles.root>
    <TooltipConfigContextProvider value=tooltipConfig>
      <HeaderBar
        onLogin={_ => setShowLogin(_ => true)}
        onSettings={_ => setShowSettings(_ => true)}
      />
      {isLanguageLoaded
         ? <div className=Styles.body>
             {switch (url.path) {
              | ["catalog"] => <MyCatalogPage />
              | ["friends", ..._] => <FriendsPage />
              | ["lists", ..._] => <MyListsPage />
              | ["u", username, ...urlRest] =>
                <UserPage
                  username
                  urlRest
                  url
                  showLogin={() => setShowLogin(_ => true)}
                  key=username
                />
              | ["l", listId, ..._urlRest] => <ListPage listId />
              | ["password-reset"] => <PasswordResetPage url />
              | ["privacy"] => <TextPages.PrivacyPolicy />
              | ["terms"] => <TextPages.TermsOfService />
              | ["import"] =>
                <ImportPage showLogin={() => setShowLogin(_ => true)} />
              | _ =>
                <>
                  <div className=Styles.tagline>
                    {React.string(
                       "Your friendly Animal Crossing marketplace!",
                     )}
                  </div>
                  <ItemBrowser showLogin={() => setShowLogin(_ => true)} url />
                </>
              }}
           </div>
         : React.null}
      <Footer />
      <QuicklistOverlay />
      {showLogin
         ? <LoginOverlay onClose={() => setShowLogin(_ => false)} />
         : React.null}
      {showSettings
         ? <SettingsOverlay onClose={() => setShowSettings(_ => false)} />
         : React.null}
      {switch (itemDetails) {
       | Some((item, variant)) =>
         <ItemDetailOverlay
           item
           variant
           isInitialLoad={React.Ref.current(isInitialLoadRef)}
         />
       | None => React.null
       }}
    </TooltipConfigContextProvider>
    <ReactAtmosphere.LayerContainer />
  </div>;
};