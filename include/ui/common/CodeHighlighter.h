#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QList>

/// Regex-based syntax highlighter for the code editors (shader, Lua script,
/// HTML). Parented to the document it highlights, so no ownership management
/// is needed at the call site.
class CodeHighlighter : public QSyntaxHighlighter {
public:
    enum class Language { Glsl, Lua, Html };

    CodeHighlighter(Language lang, QTextDocument *doc);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void setupGlsl();
    void setupLua();
    void setupHtml();

    QList<Rule> m_rules;
    QRegularExpression m_blockCommentStart;
    QRegularExpression m_blockCommentEnd;
    QTextCharFormat m_commentFormat;
};
